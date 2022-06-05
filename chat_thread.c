/* programme (c) M de Beaudrap
*/

#include <stdio.h>
#include <stdarg.h>
#include "pse.h"
#include "fifo_thread_lib.h"
#include <regex.h>
#include <unistd.h>

#define END_TRANSMIT 0x01
#define CLEAR_TRANSMIT 0x02
#define BUF_SIZE 1024
#define PRINT_LINE_MAX LIGNE_MAX + 70
#define NB_LINE_PRINT 20

typedef struct 
{
  fifo_thread_t* print_buff;
  fifo_thread_t* log_buff;
  char dist_addr_str[30];
  char dist_port_str[8];
  char local_port_str[8];
  char pseudo_distant[30];
  char pseudo_local[30];

  pthread_t pthread_rcv, pthread_send;
  int sock;

}com_struct;


void* thread_pop_fifo_print_f(void *arg);
void* thread_pop_fifo_log_f(void *arg);

void* thread_send_f(void* arg);
void* thread_rcv_f(void* arg);
int check_cmd(char* line);
int printf_chat(char* username, const char *__restrict__ __format, ...);

void create_client(com_struct* cli);
void create_serveur(com_struct* serv);

int connect_socket(fifo_thread_t* fifo_print, const char* addr_str, const char* port_str);
int create_socket(fifo_thread_t* fifo_print, const char *port_str);


int nb_print = 0;
int nb_log = 0;

char pseudo_local[30] =   "local--";
char log_file_str[60] = "journal.log";
int log_file;

int run = 1; 


int main(int argc, char *argv[]) {
  int nocli = 0;
  int noserv = 0;
  run = 1; 
  char line_to_print[PRINT_LINE_MAX];


  pthread_t pthread_fifo_print, pthread_fifo_log;
  fifo_thread_t fifo_print; // fifo pour l'ecriture sur le terminal
  fifo_thread_t fifo_log; // fifo pour l'ecriture dans le journal
  
  init_fifo_thread(&fifo_log, NB_LINE_PRINT, PRINT_LINE_MAX); // initialisation de la fifo de log
  init_fifo_thread(&fifo_print, NB_LINE_PRINT, PRINT_LINE_MAX); // initialisation de la fifo de print

  com_struct com_s = {&fifo_print, &fifo_log, "localhost", "2000", "2000", "distant", "local--", NULL, NULL, -1}; // initialisation structure du client


  regex_t  arg_ex;
  regmatch_t arg_match[2];
  regcomp(&arg_ex, "^-(\\w)$", REG_EXTENDED|REG_NOSUB);//regex qui cherche les -x (x lettre associée au paramètre)
  for(int i = 1; i<argc; i++) // ----------  parsing des arguments -------
  {
    if( regexec(&arg_ex, argv[i], 1, arg_match, 0) == 0)
    {
     
      switch (argv[i][1])
      {
      case 'a':
        if(i < argc-1)strcpy(com_s.dist_addr_str, argv[i+1]);
        break;
      case 'p':
        if(i < argc-1)strcpy(com_s.dist_port_str, argv[i+1]);
        break;
      case 'l':
        if(i < argc-1)strcpy(com_s.local_port_str, argv[i+1]);
        break;
      case 'u':
        if(i < argc-1)strcpy(com_s.pseudo_local, argv[i+1]);
        break;
      case 'o':
        if(i < argc-1)strcpy(log_file_str, argv[i+1]);
        break;
      case 'i':
        if(i < argc-1)strcpy(com_s.pseudo_distant, argv[i+1]);
        break;
      case 'c':
        nocli = 1;
        break;
      case 's':
        noserv = 1;
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
        break;
      
      default:
        break;
      }
    }


  }


if(run)
{



  pthread_create(&pthread_fifo_print, NULL, &thread_pop_fifo_print_f, &fifo_print);
  pthread_create(&pthread_fifo_log, NULL, &thread_pop_fifo_log_f, &fifo_log);



  //  creation des threads client et serveur
  printf("creating thread\n");
  if(!nocli)
  {
    printf("creating client\n");
    pthread_create(&com_s.pthread_send, NULL, &thread_send_f, &com_s);
  }
  if(!noserv)
  {
    printf("creating serveur\n");
    pthread_create(&com_s.pthread_rcv, NULL, &thread_rcv_f, &com_s);
  }
  printf("thread created\n");


  while(run)
  { 
    usleep(100);
  }

}


}


void* thread_mk_cli(void *arg)
{
}

void* thread_mk_serv(void *arg)
{
}

void* thread_pop_fifo_print_f(void *arg)
{
  fifo_thread_t* fifo_print = (fifo_thread_t*)arg;
  char line_to_print[PRINT_LINE_MAX];
  while(run)
  {
    if(pop_fifo_thread(fifo_print, line_to_print))
      printf_chat(pseudo_local, line_to_print);
  }
}

void* thread_pop_fifo_log_f(void *arg)
{
  fifo_thread_t* fifo_log = (fifo_thread_t*)arg;
  char line_to_log[PRINT_LINE_MAX];
  log_file = open(log_file_str,O_CREAT|O_WRONLY|O_APPEND, 0644); // ouverture du fichier de log

  if (log_file == -1) {
    char errmessage[90];
    sprintf(errmessage, "open output, unable to open \"%s\"", log_file_str);
    erreur_IO(errmessage);
  }
  else
  {
    while(run)
    {
      if(pop_fifo_thread(fifo_log, line_to_log))
        write(log_file, line_to_log, strlen(line_to_log));
    }
  }

  if (close(log_file) == -1) {
    erreur_IO("close output");
  }
}

 // thread envoie des messages
void* thread_send_f(void* arg)
{
  com_struct* com = (com_struct*) arg; // cast de la structure
  
  char CMD[] ="client";
  sprintf_fifo_thread(com->print_buff,"%s: thread cli start\n", CMD); //print "thread cli dans start"


  ssize_t nbRead, nbRead_tot = 0;

  char ligne[LIGNE_MAX];
  int lgEcr;
  if(com->sock == -1)
  {
    create_client(arg);
    
    if(com->pthread_rcv == NULL) 
    {
      pthread_create(&com->pthread_rcv, NULL, &thread_rcv_f, com);

      sprintf_fifo_thread(com->print_buff,"%s: serv created with socket : %d\n", CMD, com->sock); 
    }
  }
  else
  {
    sprintf_fifo_thread(com->print_buff,"%s: thread cli started by serveur, with socket : %d\n", CMD, com->sock);
  }

while(run)
{
  if (fgets(ligne, LIGNE_MAX, stdin) == NULL)
  {
    sprintf_fifo_thread(com->print_buff,"arret par CTRL-D\n");
  }
  else
  {
    lgEcr = ecrireLigne(com->sock, ligne); // send the message
    sprintf_fifo_thread(com->log_buff,"<%s>:%s", pseudo_local, ligne); // log the message
    sprintf_fifo_thread(com->print_buff,"");// renew username at the begining of the line

    if (lgEcr == -1)
      erreur_IO("ecrireLigne");

    nbRead = strlen(ligne); 
    nbRead_tot += nbRead;

    int res_regex = check_cmd(ligne);

    if(res_regex & END_TRANSMIT)
    {
      run = 0;
      sprintf_fifo_thread(com->print_buff,"%s: End sended, close serveur\n", CMD);
    }
    if(res_regex & CLEAR_TRANSMIT)
    {
      sprintf_fifo_thread(com->print_buff,"\nclean\n");
    }

  }

}

  if (close(com->sock) == -1)
    erreur_IO("close");
  
  
  sprintf_fifo_thread(com->print_buff,"%s: %ld char have been transfered\n",CMD, nbRead_tot);
  return NULL;
}


// thread reception des messages
void* thread_rcv_f(void* arg)
{
  
  com_struct* com = (com_struct*) arg;
  

  char CMD[] ="serveur";
  sprintf_fifo_thread(com->print_buff,"%s: thread serv start\n", CMD);
  
  ssize_t lgLue, lgLue_tot = 0;


  char ligne[LIGNE_MAX];

  int ecoute = -1;

  if(com->sock == -1)
  {
    create_serveur(arg);

    if(com->pthread_send == NULL)
    {
      pthread_create(&com->pthread_send, NULL, &thread_send_f, com);
      sprintf_fifo_thread(com->print_buff,"%s: client created with socket : %d\n", CMD, com->sock); 
    }
  }
  else
  {
    sprintf_fifo_thread(com->print_buff,"%s: thread serv started by client with socket : %d\n", CMD, com->sock);
  }

  while (run)
  {
    lgLue = lireLigne(com->sock, ligne);
    
    if(lgLue > 0)
    {
      lgLue_tot+= lgLue;
      if(ligne[lgLue-1] == '\0')
      {
        ligne[lgLue-1] = '\n';
        ligne[lgLue] = '\0';
      }
      
      sprintf_fifo_thread(com->print_buff,"<%s>:%s", com->pseudo_distant, ligne);
      sprintf_fifo_thread(com->log_buff,"<%s>:%s", com->pseudo_distant, ligne);

      int res_regex = check_cmd(ligne);

      if(res_regex & END_TRANSMIT)
      {
        run = 0;
        sprintf_fifo_thread(com->print_buff,"%s: End recieved, close serveur\n", CMD);
      }
      if(res_regex & CLEAR_TRANSMIT)
      {
        sprintf_fifo_thread(com->print_buff,"\nclean\n");
      }

    }
  }

  
  if (close(com->sock) == -1)
    erreur_IO("close canal");

  if(ecoute != -1)
    if (close(ecoute) == -1)
      erreur_IO("close ecoute");

  //////////////////
  
  sprintf_fifo_thread(com->print_buff,"%s: %ld char have been transfered\n",CMD, lgLue_tot);
  return NULL;
}


// virification de commande dans un message
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

// eccrire sur le terminal
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

// creer un socket de connection à un serveur
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
  sprintf_fifo_thread(fifo_print,"%s:.. connected\n", CMD);
  return sock;
}

// creer un secket d'ecoute serveur
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


// connection à un serveur distant
void create_client( com_struct* cli)
{

  cli->sock = connect_socket(cli->print_buff, cli->dist_addr_str, cli->dist_port_str);


  sprintf_fifo_thread(cli->print_buff,"client maker: connected to : adr %s, port %s\n",
        cli->dist_addr_str,
        cli->dist_port_str);


}

// creer un point d'ecoute serveur et attend une connexion (bloquant)
void create_serveur(com_struct* serv)
{

  int ecoute = create_socket(serv->print_buff, serv->local_port_str);

  struct sockaddr_in adrClient;
  unsigned int lgAdrClient;
  lgAdrClient = sizeof(adrClient);
  serv->sock = accept(ecoute, (struct sockaddr *)&adrClient, &lgAdrClient);
  if (serv->sock < 0)
    erreur_IO("accept");
 // TODO rapatrier les informations du client dans la structure com
  sprintf_fifo_thread(serv->print_buff,"serveur maker: connected to : adr %s, listen_port %d\n",
        stringIP(ntohl(adrClient.sin_addr.s_addr)),
        ntohs(adrClient.sin_port));

}
