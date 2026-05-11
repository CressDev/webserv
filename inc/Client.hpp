#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "ServerSocket.hpp"
# include <algorithm>
# include <sstream>
# include <sys/stat.h>
# include <sys/types.h>
# include <sys/wait.h>

struct HeaderContent
{
	std::string	method;
	size_t		ContentLength;
	bool		isChunked;
	std::string path;
	std::string requestUri;
	std::string	protocol;
	std::string	root;
	std::string	host;
	bool		isAutoindexResponse;
};

struct CgiState
{
    pid_t		pid;
    int			in_fd;
    int			out_fd;   
    std::string	write_buf;
    size_t		write_pos;
    std::string	read_buf;
    bool		in_closed;
    bool		out_closed;
	bool		finalized;
};

class Client
{
private:
	Server					_listener; //esto debería ser un puntero, pero resolverlo sería un cacao así que así se queda. Está feo, pero así se gestionan mejor los leaks.
	int						_fd;
	char					_buffer[4096]; // establecer una macro para el tamaño del buffer
	std::string				_request[2];
	int						_status;
	std::string				_header;
	std::string				_body;
	bool					_isHeaderReady;
	bool					_isBodyReady;
	struct HeaderContent	_headerContent;
	struct CgiState			_cgi;
	size_t					_chunkLen;
	std::string				_chunkLine;
	std::string				_sendBuffer;
	std::string				_cgiContentType;
	bool					_isSent;
	LocationConfig			_location;
	bool					_isLocation;
	std::string				_errorResolvedPath;
	std::string				_redirectLocation;
	bool					_hasErrorPageResolved;
public:
	char	**env;
	/**
	 * @brief Establece el entorno de ejecución (variables de entorno) para el cliente/CGI.
	 * @param envp Puntero al arreglo de variables de entorno (terminado en NULL).
	 */
	void	setEnv(char **envp);

	/**
	 * @brief Construye un objeto Client asociado a un listener (Server) y un descriptor de archivo.
	 * @param listener Referencia al Server que acepta la conexión.
	 * @param fd Descriptor de fichero del socket del cliente.
	 */
	Client(Server& listener, int fd);

	/**
	 * @brief Destructor del cliente; cierra recursos si es necesario.
	 */
	~Client();

	/**
	 * @brief Lee datos del socket y rellena el búfer de cabecera hasta completarla.
	 * Debe actualizar _isHeaderReady cuando la cabecera está completa.
	 */
	void		chargeHeader();

	/**
	 * @brief Lee el cuerpo del mensaje según lo que falte y actualiza el estado.
	 * Utiliza _headerContent para decidir cuánto leer.
	 */
	void		chargeBody();

	/**
	 * @brief Comprueba y valida el tamaño del contenido leído frente a Content-Length.
	 * @param num Número de bytes leídos o a comprobar.
	 */
	void		checkContentLength(size_t num);

	/**
	 * @brief Gestiona la recepción de cuerpos con codificación chunked.
	 */
	void		chunkManagement();

	/**
	 * @brief Parsea la cabecera HTTP almacenada en _header y rellena HeaderContent.
	 */
	void		parseHeader();

	/**
	 * @brief Establece el descriptor de fichero del cliente.
	 * @param fd Descriptor a asignar.
	 */
	void		setClientFd(int fd);

	/**
	 * @brief Devuelve el descriptor de fichero del cliente.
	 * @return Descriptor de fichero.
	 */
	int			getClientFd() const;

	/**
	 * @brief Indica si la cabecera HTTP ya ha sido recibida por completo.
	 * @return true si la cabecera está lista.
	 */
	bool		getIsHeaderReady() const;

	/**
	 * @brief Indica si el cuerpo HTTP ha sido recibido por completo.
	 * @return true si el cuerpo está listo.
	 */
	bool		getIsBodyReady() const;

	/**
	 * @brief Devuelve el Server (listener) asociado a este cliente.
	 * @return Copia del Server asociado.
	 */
	Server		getListener() const;

	/**
	 * @brief Devuelve el método HTTP de la petición actual (GET, POST, etc.).
	 * @return Método como string.
	 */
	std::string	getMethod() const;

	/**
	 * @brief Calcula y establece la ruta solicitada en la estructura interna.
	 */
	void		setPath();

	/**
	 * @brief Comprueba si un método está permitido comparándolo con una lista.
	 * @param method Método a comprobar.
	 * @param vec Vector con métodos permitidos.
	 * @return true si está permitido.
	 */
	bool		checkMethod(const std::string& method, const std::vector<std::string>& vec) const;

	/**
	 * @brief Verifica la validez de la ruta solicitada y selecciona index/autoindex si aplica.
	 * @param path Ruta recibida (puede ser modificada para normalizar).
	 * @param index Vector de archivos index disponibles.
	 * @param autoindex Indica si se permite generar listado de directorio.
	 * @param root Ruta raíz del servidor o location.
	 */
	void		checkPathValidity(std::string& path, std::vector<std::string>& index, bool autoindex, const std::string& root);

	/**
	 * @brief Determina si la petición debe ser manejada por CGI y lanza el proceso si procede.
	 * @return true si se inició o se gestionó CGI, false en caso contrario.
	 */
	bool		handleCgiIfNeeded();

	/**
	 * @brief Une dos componentes de ruta respetando separadores.
	 * @param a Primer componente.
	 * @param b Segundo componente.
	 * @return Ruta combinada.
	 */
	std::string	joinPath(const std::string& a, const std::string& b);

	/**
	 * @brief Inicia un CGI en modo no bloqueante usando fork/exec y pipes.
	 * @param scriptPath Ruta al script a ejecutar.
	 * @param interpreter Intérprete a usar (por ejemplo "/usr/bin/python").
	 * @return true si el proceso CGI fue lanzado correctamente.
	 */
	bool		startCgiNonBlocking(const std::string& scriptPath, const std::string& interpreter);

	/**
	 * @brief Resuelve errores HTTP (404, 500, etc.) y prepara la respuesta adecuada.
	 */
	void		handleErrors();

	/**
	 * @brief Carga la página de error por defecto configurada para este servidor.
	 * @return Ruta o contenido de la página de error por defecto.
	 */
	std::string	chargeDefaultErrorPage();

	/**
	 * @brief Asigna páginas de error específicas desde una tabla inmutable.
	 * @param errorPages Mapa con claves (grupo,código) y ruta de la página.
	 */
	void		chargeStatusData(const std::map<std::pair<int, int>, std::string>& errorPages);

	/**
	 * @brief Asigna páginas de error específicas desde una tabla mutable.
	 * @param errorPages Mapa con claves (grupo,código) y ruta de la página.
	 */
	void		chargeStatusData(std::map<std::pair<int, int>, std::string>& errorPages);

	/**
	 * @brief Maneja eventos de I/O en los descriptores asociados al CGI.
	 * @param fd Descriptor afectado.
	 * @param revents Máscara de eventos (POLLIN/POLLOUT, etc.).
	 */
	void		handleCgiFdEvent(int fd, short revents);

	/**
	 * @brief Finaliza y limpia recursos del CGI cuando ha terminado completamente.
	 */
	void		finalizeCgiIfDone();

	/**
	 * @brief Devuelve el descriptor de escritura hacia el proceso CGI (stdin del CGI).
	 * @return Descriptor in_fd del CGI.
	 */
	int			getCgiInFd() const;

	/**
	 * @brief Devuelve el descriptor de lectura desde el proceso CGI (stdout del CGI).
	 * @return Descriptor out_fd del CGI.
	 */
	int			getCgiOutFd() const;

	/**
	 * @brief Indica si hay un proceso CGI en ejecución para esta petición.
	 * @return true si está corriendo.
	 */
	bool		isCgiRunning() const;

	/**
	 * @brief Indica si la respuesta ya fue enviada completamente al cliente.
	 * @return true si todo el contenido ha sido despachado.
	 */
	bool		getIsSent() const;

	/**
	 * @brief Ensambla y envía la respuesta HTTP (cabeceras + cuerpo) al cliente.
	 */
	void		sendResponse();

	/**
	 * @brief Carga el contenido de un fichero en memoria.
	 * @param filename Ruta al fichero a leer.
	 * @return Contenido del fichero como string.
	 */
	std::string	loadContent(const std::string& filename) const;

	/**
	 * @brief Fuerza el volcado del buffer de salida al socket del cliente.
	 */
	void		flushResponse();

	/**
	 * @brief Genera una lista HTML del contenido de un directorio para autoindex.
	 * @param dirPath Ruta en el sistema de ficheros del directorio.
	 * @param requestPath Ruta usada en la petición para construir enlaces.
	 * @return HTML con el listado de directorio.
	 */
	std::string	generateDirectoryListing(const std::string& dirPath, const std::string& requestPath);

	/**
	 * @brief Maneja peticiones HTTP DELETE, borrando recursos cuando está permitido.
	 */
	void		handleDelete();

	/**
	 * @brief Devuelve el valor de la cabecera Host de la petición.
	 * @return Host como string.
	 */
	std::string getHeaderHost() const;

	/**
	 * @brief Actualiza el Server (listener) asociado al cliente.
	 * @param s Server a asociar.
	 */
	void		setListener(const Server& s);

	/**
	 * @brief Función auxiliar ejecutada en el proceso padre durante la gestión de CGI.
	 * @param inpipe Pipe de entrada (para escribir al hijo).
	 * @param outpipe Pipe de salida (para leer del hijo).
	 * @param pid PID del proceso hijo.
	 * @return true si la configuración y seguimiento del proceso padre fue satisfactoria.
	 */
	bool		parentProcess(int *inpipe, int *outpipe, pid_t pid);

};

/*Demasiadas responsabilidades en Client
Client parsea HTTP, gestiona CGI, sirve ficheros, genera directory listing, maneja errores y envía respuestas.
Para 42 es habitual, pero si hay bugs es difícil aislarlos. Mencionarlo para la evaluación.
Pasa el siguiente método cuando quieras.*/
#endif