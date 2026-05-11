#ifndef PARSER_HPP
# define PARSER_HPP

# include <string>
# include <vector>
# include <fstream>
# include <iostream>
# include <map>
# include <set>
# include <sstream>
# include <iterator>
# include <limits>
# include <unistd.h>
# include <cstdlib>
# include "utils.hpp"
# include "Server.hpp"

const std::string   configkeys[16] =
{
    "listen",
    "server_name",
    "error_page",
    "client_max_body_size",
    "root",
    "location",
    "index",
    "autoindex",
    "return",
    "upload_store",
    "upload_pass",
    "cgi",
    "alias",
    "allowed_methods" 
};

const std::string   cgikeys[4] =
{
    ".py",
    ".php",
    ".pl",
	".bla"
};

// Declaraciones de funciones de estado usadas por el parser (implementadas en srcs/parse/parser_states.cpp)
int getState(int prev, int pos);
int isConfigWord(std::string& token);

enum    State
{
    S_INI,
    S_ERR,
    S_SER,
    S_SOP,
    S_SCL,
    S_LOC,
    S_LUR,
    S_LOP,
    S_LCL,
    S_KEY,
    S_PAR,
    S_SEM
};

enum    IPV4State
{
    IP_ER,
    IP_00,
    IP_01,
    IP_02,
    IP_03,
    IP_05,
    IP_PO,
    IP_NU
};

enum    bodySizeState
{
    BD_ERR,
    BD_NUM,
    BD_CHR
};

enum    errorpageState
{
    EP_ERR,
    EP_COD,
    EP_OVR,
    EP_URI
};

enum    locationState
{
    LO_ERR,
    LO_EQU,
    LO_PAT
};

class Parser
{
    private:
        std::string                             _config_file;
        std::vector<std::string>                _tokens;
        std::multimap<std::string, std::string> _listens;
        int                                     _serverCounter;
		std::ifstream							_infile;

        /**
         * @brief Parsea la directiva "listen" y la asigna al servidor.
         * @param it Iterador sobre el vector de tokens, posicionado en el token que representa la directiva.
         * @param server Referencia al objeto Server que será modificado.
         */
        void                                    listenParser(std::vector<std::string>::iterator& it, Server& server);

        /**
         * @brief Parsea la directiva "autoindex" para un servidor.
         * @param it Iterador sobre los tokens, posicionado en la directiva.
         * @param server Servidor objetivo donde se aplicará la opción.
         */
        void                                    autoindexParser(std::vector<std::string>::iterator& it, Server& server);

        /**
         * @brief Parsea la directiva "allowed_methods" para un servidor.
         * @param it Iterador sobre los tokens.
         * @param server Servidor objetivo donde se almacenarán los métodos permitidos.
         */
        void                                    allowedParser(std::vector<std::string>::iterator& it, Server& server);

        /**
         * @brief Parsea la directiva "client_max_body_size" y la valida.
         * @param it Iterador sobre los tokens.
         * @param server Servidor donde se establecerá el tamaño máximo de cuerpo.
         */
        void                                    clientmaxbodysizeParser(std::vector<std::string>::iterator& it, Server& server);

        /**
         * @brief Parsea la directiva "server_name" y la añade a la configuración del servidor.
         * @param it Iterador sobre los tokens.
         * @param server Servidor donde se añadirán los nombres.
         */
        void                                    serverNameParser(std::vector<std::string>::iterator& it, Server& server);

        /**
         * @brief Parsea la directiva "error_page" y la asocia a códigos HTTP.
         * @param it Iterador sobre los tokens.
         * @param server Servidor que recibirá la configuración de páginas de error.
         */
        void                                    errorpageParser(std::vector<std::string>::iterator& it, Server& server);

        /**
         * @brief Parsea la directiva "cgi" y registra las rutas/handlers asociados.
         * @param it Iterador sobre los tokens.
         * @param server Servidor que recibirá la configuración CGI.
         */
        void                                    cgiParser(std::vector<std::string>::iterator& it, Server& server);

        /**
         * @brief Parsea la directiva "root" para un servidor.
         * @param it Iterador sobre los tokens.
         * @param server Servidor donde se asignará la ruta raíz.
         * @param n Índice o flag usado internamente para diferenciar casos (archivo/loc).
         */
        void                                    rootParser(std::vector<std::string>::iterator& it, Server& server, int n);

        /**
         * @brief Parsea la directiva "root" dentro de una definición de location.
         * @param it Iterador sobre los tokens.
         * @param loc Configuración de la location que será modificada.
         * @param i Índice o flag auxiliar usado por el parser de location.
         */
        void                                    rootLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc, int i);

        /**
         * @brief Parsea la directiva "index" dentro de una location.
         * @param it Iterador sobre los tokens.
         * @param loc LocationConfig que almacenará los índices.
         */
        void	                                indexLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc);

        /**
         * @brief Parsea la configuración CGI específica dentro de una location.
         * @param it Iterador sobre los tokens.
         * @param loc LocationConfig que recibirá la configuración CGI.
         */
        void                                    cgiLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc);

        /**
         * @brief Parsea la directiva "autoindex" dentro de una location.
         * @param it Iterador sobre los tokens.
         * @param loc LocationConfig objetivo.
         */
        void							autoindexLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc);

        /**
         * @brief Parsea "allowed_methods" dentro de una location.
         * @param it Iterador sobre los tokens.
         * @param loc LocationConfig que almacenará los métodos permitidos.
         */
        void							allowedLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc);

        /**
         * @brief Parsea "error_page" dentro de una location.
         * @param it Iterador sobre los tokens.
         * @param loc LocationConfig que recibirá las entradas de páginas de error.
         */
        void							errorPageLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc);

        /**
         * @brief Parsea una definición de "location" completa y la añade al servidor.
         * @param it Iterador sobre los tokens, posicionado al inicio de la definición de location.
         * @param server Servidor al que se añadirá la location.
         */
        void                                    locationParser(std::vector<std::string>::iterator& it, Server& server);

        /**
         * @brief Parsea combinaciones/otros campos dentro de una location (helper interno).
         * @param it Iterador sobre los tokens.
         * @param loc LocationConfig que se está construyendo.
         */
        void						cmbsLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc);

        /**
         * @brief Comprueba que no existan conflictos entre server_name de distintos servidores.
         * @param servers Vector de servidores a validar.
         */
        void    						checkServerNames(std::vector<Server>& servers);

        /**
         * @brief Obtiene el estado de análisis para server_name (estado previo y posición).
         * @param prev Estado previo.
         * @param pos Posición actual en el análisis.
         * @return Nuevo estado calculado.
         */
        int			                        getServerNameState(int prev, int pos);

        /**
         * @brief Valida el formato del valor de client_max_body_size.
         * @param t Token que contiene el valor a validar.
         * @return true si es válido, false en caso contrario.
         */
        bool                                    checkclientmaxbodysize(std::string t);

        /**
         * @brief Valida una dirección IPv4 en tokens de configuración.
         * @param t Token con la posible dirección IPv4.
         * @return true si tiene formato IPv4 válido, false en caso contrario.
         */
        bool                                    check_ipv4(std::string t);

        /**
         * @brief Valida un puerto TCP en tokens de configuración.
         * @param t Token con el número de puerto.
         * @return true si el puerto es válido (0-65535), false en caso contrario.
         */
        bool                                    check_port(std::string t);

        /**
         * @brief Determina el estado inicial/actual del parser basado en los tokens.
         * @param tokens Vector de tokens de la configuración.
         * @return Estado elegido para el parser.
         */
        int                                     chooseState(std::vector<std::string>& tokens);

        /**
         * @brief Avanza/obten el estado del parser para IPv4 durante el análisis.
         * @param prev Estado previo.
         * @param pos Posición actual.
         * @return Nuevo estado IPV4.
         */
        int                                     getIPV4State(int prev, int pos);

        /**
         * @brief Obtiene el estado de análisis del valor client_max_body_size.
         * @param prev Estado previo.
         * @param pos Posición actual.
         * @return Nuevo estado para el parser de client_max_body_size.
         */
        int                                     getClientMaxBodySizeState(int prev, int pos);

        /**
         * @brief Obtiene el estado para parser de error_page.
         * @param prev Estado previo.
         * @param pos Posición actual.
         * @return Nuevo estado del parser de error_page.
         */
        int                                     getErrorPageParserState(int prev, int pos);

        /**
         * @brief Determina el tipo de un ítem/token de configuración.
         * @param str Token a evaluar.
         * @return Entero que representa el tipo identificado.
         */
        int                                     getTypeOfItem(std::string& str);

        /**
         * @brief Parsea la directiva "index" a nivel de servidor.
         * @param it Iterador sobre los tokens.
         * @param server Servidor que almacenará la configuración de índices.
         */
        void                                    indexParser(std::vector<std::string>::iterator& it, Server& server);

        /**
         * @brief Elige el estado interno correspondiente para la directiva index.
         * @param str Token con el valor de index.
         */
        void	                                chooseIndexState(std::string str);

        /**
         * @brief Obtiene el estado del parser relacionado con index.
         * @param prev Estado previo.
         * @param pos Posición actual.
         * @return Nuevo estado para index.
         */
        int                                     getIndexState(int prev, int pos);

        /**
         * @brief Obtiene el estado de análisis de una sección location.
         * @param prev Estado previo.
         * @param pos Posición actual.
         * @return Nuevo estado para location.
         */
        int                                     getLocationState(int prev, int pos);

        /**
         * @brief Valida que las directivas "listen" de los servidores sean coherentes y completas.
         * @param servers Vector de servidores a comprobar.
         */
        void                                    checkListen(std::vector<Server>& servers);

    public:
        /**
         * @brief Construye un Parser y comienza el proceso de parsing sobre el archivo dado.
         * @param in_file Ruta al archivo de configuración.
         * @param servers Vector de servidores que será completado durante el parseo.
         */
        Parser(const char* in_file, std::vector<Server>& servers);

        /**
         * @brief Destructor del parser, libera recursos si es necesario.
         */
        ~Parser();

        /**
         * @brief Tokeniza el contenido del archivo de configuración.
         * @return 0 en éxito, otro valor en caso de error.
         */
        int                         tokenize();

        /**
         * @brief Elimina comentarios del stream del archivo de configuración.
         * @param config_file Stream del archivo de configuración.
         */
        void                        rmComments(std::ifstream& config_file);

        /**
         * @brief Devuelve el vector de tokens extraídos tras la tokenización.
         * @return Vector de strings con los tokens.
         */
        std::vector<std::string>    get_tokens();
};

#endif