/* programme (c) M de Beaudrap
*/

#include <stdio.h>
#include <stdarg.h>
#include "pse.h"
#include "fifo_thread_lib.h"
#include <regex.h>


#define END_TRANSMIT 0x01
#define CLEAR_TRANSMIT 0x02
#define BUF_SIZE 1024
#define PRINT_LINE_MAX LIGNE_MAX + 70
#define NB_LINE_PRINT 20

typedef struct
{
  fifo_thread_t* print_buff;
  fifo_thread_t* log_buff;
  char addr_str[30];
  char port_str[8];


  struct sockaddr_in *adrServ;

  //cli_struct* next;  // TODO chainer les instances
}cli_struct;

typedef struct 
{
  fifo_thread_t* print_buff;
  fifo_thread_t* log_buff;
  char port_str[8];
  char pseudo_distant[30];

}serv_struct;



void* thread_cli_f(void* arg);
void* thread_serv_f(void* arg);
int check_cmd(char* line);
int printf_chat(char* username, const char *__restrict__ __format, ...);

int connect_socket(fifo_thread_t* fifo_print, const char* addr_str, const char* port_str);
int create_socket(fifo_thread_t* fifo_print, const char *port_str);


  int nb_print = 0;
  int nb_log = 0;

  char pseudo_local[30] =   "local--";
  char log_file_str[60] = "journal.log";
  int log_file;


int main(int argc, char *argv[]) {

  int run = 1; 
  char line_to_print[PRINT_LINE_MAX];
  pthread_t pthread_cli, pthread_serv;


  fifo_thread_t fifo_log;
  fifo_thread_t fifo_print;
  init_fifo_thread(&fifo_log, NB_LINE_PRINT, PRINT_LINE_MAX);
  init_fifo_thread(&fifo_print, NB_LINE_PRINT, PRINT_LINE_MAX);

  serv_struct serveur_s = {&fifo_print, &fifo_log, "2000", "distant"};
  cli_struct client_s = {&fifo_print, &fifo_log, "localhost", "2000"};

  regex_t  arg_ex;
  regmatch_t arg_match[2];
  regcomp(&arg_ex, "^-(\\w)$", REG_EXTENDED|REG_NOSUB);//regex qui cherche les -x (x lettre associée au paramètre)
  for(int i = 1; i<argc; i++)
  {
    if( regexec(&arg_ex, argv[i], 1, arg_match, 0) == 0)
    {
     
      switch (argv[i][1])
      {
      case 'a':
        if(i < argc-1)strcpy(client_s.addr_str, argv[i+1]);
        break;
      case 'p':
        if(i < argc-1)strcpy(client_s.port_str, argv[i+1]);
        break;
      case 'l':
        if(i < argc-1)strcpy(serveur_s.port_str, argv[i+1]);
        break;
      case 'u':
        if(i < argc-1)strcpy(pseudo_local, argv[i+1]);
        break;
      case 'o':
        if(i < argc-1)strcpy(log_file_str, argv[i+1]);
        break;
      case 'i':
        if(i < argc-1)strcpy(serveur_s.pseudo_distant, argv[i+1]);
        break;
      case 'h':
        printf("%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s","Help : ",
        "-a : addresse de l'interlocuteur",
        "-p : port de l'interlocuteur",
        "-l : port d'ecoute local",
        "-u : mon username",
        "-o : fichier de log",
        "-i : username de l'interlocuteur");
        run = 0;
        /* code */
        break;
      
      default:
        break;
      }
    }


  }


if(run)
{



  log_file = open(log_file_str,O_CREAT|O_WRONLY|O_APPEND, 0644);
  if (log_file == -1) {
    char errmessage[90];
    sprintf(errmessage, "open output, unable to open \"%s\"", log_file_str);
    erreur_IO(errmessage);
  }



  printf("creating thread\n");
  pthread_create(&pthread_cli, NULL, &thread_cli_f, &client_s);
  pthread_create(&pthread_serv, NULL, &thread_serv_f, &serveur_s);
  printf("thread created\n");
  while(run)
  { 
    
    if(pop_fifo_thread(&fifo_print, line_to_print))
      printf_chat(pseudo_local, line_to_print);
    if(pop_fifo_thread(&fifo_log, line_to_print))
      write(log_file, line_to_print, strlen(line_to_print));

  }

}
  if (close(log_file) == -1) {
    erreur_IO("close output");
  }

}

void* thread_cli_f(void* arg)
{
  cli_struct* cli = (cli_struct*) arg;
  char CMD[] ="client";
  sprintf_fifo_thread(cli->print_buff,"%s: thread cli start\n", CMD);
  
  ssize_t nbRead, nbRead_tot = 0;
  int run = 1;

  char ligne[LIGNE_MAX];
  int lgEcr;

  int sock = connect_socket(cli->print_buff, cli->addr_str, cli->port_str);

while(run)
{
  if (fgets(ligne, LIGNE_MAX, stdin) == NULL)
  {
    sprintf_fifo_thread(cli->print_buff,"arret par CTRL-D\n");
  }
  else
  {
    lgEcr = ecrireLigne(sock, ligne); // send the message
    sprintf_fifo_thread(cli->log_buff,"<%s>:%s", pseudo_local, ligne); // log the message
    sprintf_fifo_thread(cli->print_buff,"");// renew username at the begining of the line

    if (lgEcr == -1)
      erreur_IO("ecrireLigne");

    nbRead = strlen(ligne); 
    nbRead_tot += nbRead;

    int res_regex = check_cmd(ligne);

    if(res_regex & END_TRANSMIT)
    {
      run = 0;
      sprintf_fifo_thread(cli->print_buff,"%s: End sended, close serveur\n", CMD);
    }
    if(res_regex & CLEAR_TRANSMIT)
    {
      sprintf_fifo_thread(cli->print_buff,"\nclean\n");
    }

  }

}

  if (close(sock) == -1)
    erreur_IO("close");
  
  
  sprintf_fifo_thread(cli->print_buff,"%s: %ld char have been transfered\n",CMD, nbRead_tot);
  return NULL;
}

void* thread_serv_f(void* arg)
{
  serv_struct* serv = (serv_struct*) arg;
  char CMD[] ="serveur";
  sprintf_fifo_thread(serv->print_buff,"%s: thread serv start\n", CMD);
  
  
  ssize_t lgLue, lgLue_tot = 0;
  int run = 1;


  char ligne[LIGNE_MAX];
  int ecoute = create_socket(serv->print_buff, serv->port_str);





  struct sockaddr_in adrClient;
  unsigned int lgAdrClient;
  int canal; 
  lgAdrClient = sizeof(adrClient);
  canal = accept(ecoute, (struct sockaddr *)&adrClient, &lgAdrClient);
  if (canal < 0)
    erreur_IO("accept");

  
  
  sprintf_fifo_thread(serv->print_buff,"%s: adr %s, listen_port %hu\n", CMD,
         stringIP(ntohl(adrClient.sin_addr.s_addr)),
         ntohs(adrClient.sin_port));

  while (run)
  {
    lgLue = lireLigne(canal, ligne);
    
    if(lgLue > 0)
    {
      lgLue_tot+= lgLue;
       usleep(10000);
      if(ligne[lgLue-1] == '\0')
      {
        ligne[lgLue-1] = '\n';
        ligne[lgLue] = '\0';
      }
      
      

      int res_regex = check_cmd(ligne);

      if(res_regex & END_TRANSMIT)
      {
        run = 0;
        sprintf_fifo_thread(serv->print_buff,"%s: End recieved, close serveur\n", CMD);
      }
      if(res_regex & CLEAR_TRANSMIT)
      {
        sprintf_fifo_thread(serv->print_buff,"\nclean\n");
      }
      sprintf_fifo_thread(serv->print_buff,"<%s>:%s", serv->pseudo_distant, ligne);
      sprintf_fifo_thread(serv->log_buff,"<%s>:%s", serv->pseudo_distant, ligne);

    }
  }

  
  if (close(canal) == -1)
    erreur_IO("close canal");

  if (close(ecoute) == -1)
    erreur_IO("close ecoute");

  //////////////////
  
  sprintf_fifo_thread(serv->print_buff,"%s: %ld char have been transfered\n",CMD, lgLue_tot);
  return NULL;
}

int check_cmd(char* line)
{
  int ret = 0;
  char fin_ex_str[] = "^fin\n?$";
  char init_ex_str[] = "^init\n?$";
  regex_t fin_ex;
  regex_t init_ex;
  regcomp(&init_ex, init_ex_str, REG_EXTENDED|REG_NOSUB);
  regcomp(&fin_ex, fin_ex_str, REG_EXTENDED|REG_NOSUB);

  if( regexec(&fin_ex, line, 0, NULL, 0) == 0)
  {
    ret |= END_TRANSMIT;
  }
  if( regexec(&init_ex, line, 0, NULL, 0) == 0)
  {
    ret |= CLEAR_TRANSMIT;
  }
  
  return ret;
}

int printf_chat(char* username, const char *__restrict__ __format, ...)
{
    int done;
    printf("\r");
    va_list args;
    va_start(args, __format);
    done = vprintf(__format, args);
    printf("<%s>:", username);
    fflush(stdout);
    return done;

}


int connect_socket(fifo_thread_t* fifo_print, const char* addr_str, const char* port_str)
{
  char CMD[] ="client:socket";
  signal(SIGPIPE, SIG_IGN);
  int sock, ret;
  sprintf_fifo_thread(fifo_print,"%s: creating a socket\n", CMD);

  struct sockaddr_in *adrServ;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    erreur_IO("socket");

  sprintf_fifo_thread(fifo_print,"%s: DNS resolving for %s, port %s\n", CMD, addr_str, port_str);
  adrServ = resolv(addr_str, port_str);
  if (adrServ == NULL)
    erreur("adresse %s port %s inconnus\n", addr_str, port_str);

  sprintf_fifo_thread(fifo_print,"%s: adr %s, port %hu\n", CMD,
	       stringIP(ntohl(adrServ->sin_addr.s_addr)),
	       ntohs(adrServ->sin_port));

  sprintf_fifo_thread(fifo_print,"%s: connecting the socket ...\n", CMD);
  ret = 0;
  while(connect(sock, (struct sockaddr *)adrServ, sizeof(struct sockaddr_in)) < 0);
  if (ret < 0)
    erreur_IO("connect");
  sprintf_fifo_thread(fifo_print,"%s:.. connecte\n", CMD);
  return sock;
}

int create_socket(fifo_thread_t* fifo_print, const char *port_str)
{
  char CMD[] ="serveur:socket";

  int ecoute, ret;
  struct sockaddr_in adrEcoute;
  short listen_port = (short)atoi(port_str);

  sprintf_fifo_thread(fifo_print,"%s: creating a socket\n", CMD);
  ecoute = socket (AF_INET, SOCK_STREAM, 0);
  if (ecoute < 0)
    erreur_IO("socket");
  
  adrEcoute.sin_family = AF_INET;
  adrEcoute.sin_addr.s_addr = INADDR_ANY;
  adrEcoute.sin_port = htons(listen_port);
  sprintf_fifo_thread(fifo_print,"%s: binding to INADDR_ANY address on port %d\n", CMD, listen_port);
  ret = bind (ecoute,  (struct sockaddr *)&adrEcoute, sizeof(adrEcoute));
  if (ret < 0)
    erreur_IO("bind");
  
  sprintf_fifo_thread(fifo_print,"%s: listening to socket\n", CMD);
  ret = listen (ecoute, 5);
  if (ret < 0)
    erreur_IO("listen");
  
  sprintf_fifo_thread(fifo_print,"%s: accepting a connection\n", CMD);
  
  return ecoute;
}

