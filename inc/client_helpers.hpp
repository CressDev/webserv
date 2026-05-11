#ifndef CLIENT_HELPERS_HPP
#define CLIENT_HELPERS_HPP

#include <string>
#include <map>

/**
 * @brief Devuelve el tipo MIME (Content-Type) según la extensión de fichero.
 *
 * @param path Ruta o nombre de archivo cuyo tipo MIME se desea obtener.
 * @return string con el tipo MIME (por ejemplo "text/html").
 */
std::string getMimeType(const std::string& path);

/**
 * @brief Traduce un código de estado HTTP a su frase estándar.
 *
 * @param code Código de estado HTTP (por ejemplo 200, 404).
 * @return const char* con la frase correspondiente (por ejemplo "OK").
 */
const char* reasonPhrase(int code);

/**
 * @brief Convierte un carácter hexadecimal en su valor numérico.
 *
 * @param c Carácter hexadecimal ('0'-'9','a'-'f','A'-'F').
 * @return int Valor numérico (0-15) o -1 si `c` no es hex.
 */
int hexVal(char c);

/**
 * @brief Decodifica una cadena URL-encoded.
 *
 * @param s Cadena codificada (por ejemplo "%20").
 * @return string Decodificada (por ejemplo ' ').
 */
std::string urlDecode(const std::string& s);

/**
 * @brief Devuelve el directorio padre de una ruta.
 *
 * @param path Ruta de entrada (por ejemplo "/a/b/c").
 * @return string Directorio padre (por ejemplo "/a/b") o cadena vacía si no existe.
 */
std::string getDirectory(const std::string& path);

/**
 * @brief Marca un descriptor como no bloqueante (O_NONBLOCK).
 *
 * @param fd Descriptor de fichero a modificar.
 * @return true si la operación tuvo éxito, false en caso contrario.
 */
bool setNonBlocking(int fd);

/**
 * @brief Construye un arreglo `char**` con variables de entorno a partir
 *        de un mapa de extras y del entorno padre.
 *
 * @param extras Mapa de variables a añadir o sobrescribir (clave->valor).
 * @param parent_env Entorno padre (char** terminado en NULL) que se
 *                   mergea con `extras`. Puede ser NULL.
 * @return char** Arreglo terminado en NULL o NULL si falla la asignación.
 */
char **buildEnvpFromMapAndParent(const std::map<std::string, std::string>& extras, char **parent_env);

/**
 * @brief Libera un arreglo `char**` creado por `buildEnvpFromMapAndParent`.
 *
 * @param envp Arreglo de C-strings terminado en NULL a liberar.
 */
void freeEnvp(char **envp);

/**
 * @brief Normaliza una ruta resolviendo "." y ".." y eliminando separadores
 *        redundantes. No interactúa con el sistema de ficheros.
 *
 * @param p Ruta de entrada a normalizar.
 * @return string Ruta normalizada.
 */
std::string normalizePath(const std::string& p);

/**
 * @brief Comprueba si una ruta candidata está bajo un directorio raíz.
 *
 * @param candidate Ruta a comprobar (puede ser absoluta o relativa).
 * @param root Directorio raíz de referencia.
 * @return true si `candidate` está dentro de `root`, false en caso contrario.
 */
bool isWithinRoot(const std::string& candidate, const std::string& root);

#endif // CLIENT_INTERNAL_HPP
