/**
 * Constants and definitions
 * @file
 */

#include <pthread.h>
#define DEF_IP		"127.0.0.1"
#define DEF_PORT	"8080"
#define DEF_ROOT	"./"

#define MAX_CLIENTS 100

#define CRLF "\r\n"
#define SPC " "
#define DELIM_URI "?"
#define DELIM_ARGS '&'

#define REQUEST_BUFFER	1024
#define BASH_BUFFER		512

#define OPT_COUNT		8
#define OPT_LENGTH		32
#define URI_LENGTH		256
#define LINE_LENGTH		512
#define VERSION_LENGTH	32

#define S_200 "HTTP/1.1 200 OK"
#define SERVER "Server: frihttp 1.0 (linux)"
#define CLOSED "Connection: Closed"

#define S_404 "HTTP/1.1 404 Not Found"
#define S_404TXTA "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>404 Not Found</title>\n</head>\n<body>\n<h1>Not Found</h1>\n<p>The requested URL "
#define S_404TXTB " was not found on this server.</p>\n</body>\n</html>"

#define S_400 "HTTP/1.1 400 Bad Request"
#define S_400TXTA "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>400 Bad Request</title>\n</head>\n<body>\n<h1>Bad Request</h1>\n<p>"
#define S_400TXTB "Your browser sent a request that this server could not understand.</p>\n</body>\n</html>"

typedef enum {GET, UNKNOWN} method_t;									/**< enum of http methods */
typedef enum {BASH, HTML, JPG, JPEG, BMP, GIF, PNG, UNK} filetype_t;	/**< enum of filetypes */

/**
 * This structure is given to threads
 */
struct thData{
	int cliFd;			/**< Clients filedescriptor */
	char * rootDir;		/**< Pointer to string with root directory */
};

/**
 * To this structure are saved data
 * about received request and parsed data.
 */
struct request_t{
	method_t method;					/**< http method from enum method_t */
	char uri[URI_LENGTH];				/**< string with uri from request*/
	char version[VERSION_LENGTH];		/**< string with version */
	char opts[OPT_COUNT][OPT_LENGTH];	/**< array of strings for bash script params */
	char *optv[OPT_COUNT + 2];			/**< array of pointers on params,[0] - file name,[n] - NULL */
	int optc;							/**< numer of params for bash script */
	filetype_t type;					/**< type of file from enum filetype_t */
	int filesize;						/**< size of file */
};
