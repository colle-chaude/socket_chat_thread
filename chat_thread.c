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
//#define    CMD      "serveur"


typedef struct
{
  char line[PRINT_LINE_MAX];
  int new_line;
  int flag;
}to_print_struct;


typedef struct
{
  fifo_thread_t* print_buff;
  fifo_thread_t* log_buff;
  char addr_str[30];
  char port_str[8];

  to_print_struct print;
  to_print_struct log;

  struct sockaddr_in *adrServ;

  //cli_struct* next;
}cli_struct;

typedef struct 
{
  fifo_thread_t* print_buff;
  fifo_thread_t* log_buff;
  char port_str[8];
  char pseudo_distant[30];


  to_print_struct print;
  to_print_struct log;

  //short listen_port;
  //serv_struct* next;
}serv_struct;



void* thread_cli_f(void* arg);
void* thread_serv_f(void* arg);
int check_cmd(char* line);
int print_line(to_print_struct* __toprint, const char *__restrict__ __format, ...);
int printf_chat(char* username, const char *__restrict__ __format, ...);

int create_socket(to_print_struct* print, char *port_str);


  to_print_struct* list_print[30];
  to_print_struct* list_log[30];
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

  list_log[nb_log++] = &serveur_s.log;
  list_log[nb_log++] = &client_s.log;

  list_print[nb_print++] = &serveur_s.print;
  list_print[nb_print++] = &client_s.print;


  for(int i = 0; i< nb_log; i++)
  {
    list_log[i]->flag = 1;
    list_log[i]->new_line = 0;
  }
  for(int i = 0; i< nb_print; i++)
  {
    list_print[i]->flag = 1;
    list_print[i]->new_line = 0;
  }


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



    for(int i = 0; i < nb_print; i++)
    {
      if(list_print[i]->flag && list_print[i]->new_line)
      {
        //printf("\r");
        //printf(list_print[i]->line);
        //printf("<%s>:", pseudo_local);
        //fflush(stdout);
        printf_chat(pseudo_local, list_print[i]->line);
        list_print[i]->new_line = 0;
      }
    }  
    for(int i = 0; i < nb_log; i++)
    {
      if(list_log[i]->flag && list_log[i]->new_line)
      {
        write(log_file, list_log[i]->line, strlen(list_log[i]->line));
        list_log[i]->new_line = 0;
      }
    }  
    
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
  print_line(&cli->print,"%s: thread cli start\n", CMD);
  
  //int inputFd, log_file;
  ssize_t nbRead, nbRead_tot = 0;
 // char buf[BUF_SIZE];
  int run = 1;

  //struct sockaddr_in *adrServ;
  char ligne[LIGNE_MAX];
  int lgEcr;



  signal(SIGPIPE, SIG_IGN);
  int sock, ret;
  print_line(&cli->print,"%s: creating a socket\n", CMD);
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    erreur_IO("socket");

  print_line(&cli->print,"%s: DNS resolving for %s, port %s\n", CMD, cli->addr_str, cli->port_str);
  cli->adrServ = resolv(cli->addr_str, cli->port_str);
  if (cli->adrServ == NULL)
    erreur("adresse %s port %s inconnus\n", cli->addr_str, cli->port_str);

  print_line(&cli->print,"%s: adr %s, port %hu\n", CMD,
	       stringIP(ntohl(cli->adrServ->sin_addr.s_addr)),
	       ntohs(cli->adrServ->sin_port));

  print_line(&cli->print,"%s: connecting the socket ...\n", CMD);
  ret = 0;
  while(connect(sock, (struct sockaddr *)cli->adrServ, sizeof(struct sockaddr_in)) < 0);
  if (ret < 0)
    erreur_IO("connect");
  print_line(&cli->print,"%s:.. connecte\n", CMD);


while(run)
{
  if (fgets(ligne, LIGNE_MAX, stdin) == NULL)
  {
    print_line(&cli->print,"arret par CTRL-D\n");
  }
  else
  {
    lgEcr = ecrireLigne(sock, ligne); // send the message
    print_line(&cli->log,"<%s>:%s", pseudo_local, ligne); // log the message
    print_line(&cli->print,"");// renew username at the begining of the line

    if (lgEcr == -1)
      erreur_IO("ecrireLigne");

    nbRead = strlen(ligne); 
    nbRead_tot += nbRead;

    int res_regex = check_cmd(ligne);

    if(res_regex & END_TRANSMIT)
    {
      run = 0;
      print_line(&cli->print,"%s: End sended, close serveur\n", CMD);
    }
    if(res_regex & CLEAR_TRANSMIT)
    {
      print_line(&cli->print,"\nclean\n");
    }

  }

}

  if (close(sock) == -1)
    erreur_IO("close");
  
  
  print_line(&cli->print,"%s: %ld char have been transfered\n",CMD, nbRead_tot);
  return NULL;
}

void* thread_serv_f(void* arg)
{
  serv_struct* serv = (serv_struct*) arg;
  char CMD[] ="serveur";
  print_line(&serv->print,"%s: thread serv start\n", CMD);
  
  
  ssize_t lgLue, lgLue_tot = 0;
  int run = 1;


  char ligne[LIGNE_MAX];
  int ecoute = create_socket(&serv->print, serv->port_str);





  struct sockaddr_in adrClient;
  unsigned int lgAdrClient;
  int canal; 
  lgAdrClient = sizeof(adrClient);
  canal = accept(ecoute, (struct sockaddr *)&adrClient, &lgAdrClient);
  if (canal < 0)
    erreur_IO("accept");

  
  
  print_line(&serv->print,"%s: adr %s, listen_port %hu\n", CMD,
         stringIP(ntohl(adrClient.sin_addr.s_addr)),
         ntohs(adrClient.sin_port));

  while (run)
  {
    lgLue = lireLigne(canal, ligne);
    
    if(lgLue > 0)
    {
      lgLue_tot+= lgLue;
      //char output[LIGNE_MAX + 65];
     // print_line(&serv->print,"%s: reception %ld octets : \"%s\"\n", CMD, lgLue, ligne);
       usleep(10000);
      if(ligne[lgLue-1] == '\0')
      {
        ligne[lgLue-1] = '\n';
        ligne[lgLue] = '\0';
      }
      
      
      //sprintf_fifo_thread(serv->log_buff,"%d_<%s>:%s", ret_sprint, serv->pseudo_distant, ligne);
      //print_line(&serv->print,"<%s>:%s", serv->pseudo_distant, ligne);
      //print_line(&serv->log,"<%s>:%s", serv->pseudo_distant, ligne);
      //print_line(&serv->print,"%s",output);
      //print_line(&serv->log,"%s",output);

      int res_regex = check_cmd(ligne);

      if(res_regex & END_TRANSMIT)
      {
        run = 0;
        print_line(&serv->print,"%s: End recieved, close serveur\n", CMD);
      }
      if(res_regex & CLEAR_TRANSMIT)
      {
        print_line(&serv->print,"\nclean\n");
      }
      int ret_sprint = sprintf_fifo_thread(serv->print_buff,"<%s>:%s", serv->pseudo_distant, ligne);
      sprintf_fifo_thread(serv->log_buff,"<%s>:%s", serv->pseudo_distant, ligne);

    }
  }

  
  if (close(canal) == -1)
    erreur_IO("close canal");

  if (close(ecoute) == -1)
    erreur_IO("close ecoute");

  //////////////////
  
  print_line(&serv->print,"%s: %ld char have been transfered\n",CMD, lgLue_tot);
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


int print_line(to_print_struct* __toprint, const char *__restrict__ __format, ...)
{
  int done = -1;
  if(__toprint->flag)
  {
    while(__toprint->new_line);
    va_list args;
    va_start(args, __format);
    done = vsprintf(__toprint->line, __format, args);
    va_end(args);
    __toprint->new_line = 1;
  }
  return done;
}

int create_socket(to_print_struct* print, char *port_str)
{
  char CMD[] ="serveur:socket";

  int ecoute, ret;
  struct sockaddr_in adrEcoute;
  short listen_port = (short)atoi(port_str);

  print_line(print,"%s: creating a socket\n", CMD);
  ecoute = socket (AF_INET, SOCK_STREAM, 0);
  if (ecoute < 0)
    erreur_IO("socket");
  
  adrEcoute.sin_family = AF_INET;
  adrEcoute.sin_addr.s_addr = INADDR_ANY;
  adrEcoute.sin_port = htons(listen_port);
  print_line(print,"%s: binding to INADDR_ANY address on port %d\n", CMD, listen_port);
  ret = bind (ecoute,  (struct sockaddr *)&adrEcoute, sizeof(adrEcoute));
  if (ret < 0)
    erreur_IO("bind");
  
  print_line(print,"%s: listening to socket\n", CMD);
  ret = listen (ecoute, 5);
  if (ret < 0)
    erreur_IO("listen");
  
  print_line(print,"%s: accepting a connection\n", CMD);
  
  return ecoute;
}

