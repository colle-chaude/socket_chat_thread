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
  fifo_thread_sub_t* input_buff;
  fifo_thread_t* log_buff;
  char dist_addr_str[30];
  char dist_port_str[8];
  char local_port_str[8];
  char pseudo_distant[30];
  char pseudo_local[30];

  pthread_t pthread_rcv, pthread_send;
  int sock;
  int running;

}com_struct_t;

typedef struct
{
  fifo_thread_t* print_buff;
  fifo_thread_sub_t* input_buff;
}term_var_struct_t;


void* thread_mk_cli(void *arg);
void* thread_mk_serv(void *arg);

void* thread_pop_fifo_print_f(void *arg);
void* thread_add_fifo_input_f(void *arg);
void* thread_pop_fifo_log_f(void *arg);

void* thread_send_f(void* arg);
void* thread_rcv_f(void* arg);
int check_cmd(char* line);
int printf_chat(char* username, const char *__restrict__ __format, ...);

void create_client(com_struct_t* cli);
int create_serveur(com_struct_t* serv);

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


  pthread_t pthread_fifo_print, pthread_fifo_input, pthread_fifo_log;
  fifo_thread_t fifo_print; // fifo for print in the terminal
  fifo_thread_sub_t fifo_input; // fifo for the input (stdin)
  fifo_thread_t fifo_log; // fifo for thpthread_fifo_inpute log file

  
  init_fifo_thread(&fifo_log, NB_LINE_PRINT, PRINT_LINE_MAX); // initialisation de la fifo de log
  init_fifo_thread(&fifo_print, NB_LINE_PRINT, PRINT_LINE_MAX); // initialisation de la fifo de print
  init_fifo_thread_sub(&fifo_input, NB_LINE_PRINT, PRINT_LINE_MAX, 5); // initialisation de la fifo de input

  pthread_t mk_cli_thread_handler, mk_serv_thread_handler;
  com_struct_t com_s = {&fifo_print, &fifo_input, &fifo_log, "localhost", "2000", "2000", "distant", "local--", NULL, NULL, -1, 1}; // initialisation structure du client
  term_var_struct_t term_var_s = {&fifo_print, &fifo_input}; // initialisation structure du terminal

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


  // trhead for with the terminal and log file
  pthread_create(&pthread_fifo_print, NULL, &thread_pop_fifo_print_f, &fifo_print);
  pthread_create(&pthread_fifo_input, NULL, &thread_add_fifo_input_f, &term_var_s);
  pthread_create(&pthread_fifo_log, NULL, &thread_pop_fifo_log_f, &fifo_log);



  //  create client or/and server
  printf("creating thread\n");
  if(!nocli)
  {
    printf("creating client\n");
    //pthread_create(&com_s.pthread_send, NULL, &thread_send_f, &com_s);
    pthread_create(&mk_cli_thread_handler, NULL, &thread_mk_cli, &com_s);
    
  }
  if(!noserv)
  {
    printf("creating serveur\n");
    //pthread_create(&com_s.pthread_rcv, NULL, &thread_rcv_f, &com_s);
    pthread_create(&mk_serv_thread_handler, NULL, &thread_mk_serv, &com_s);
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
  com_struct_t* com = (com_struct_t*) arg; 
  
  char CMD[] ="mk_cli";
  sprintf_fifo_thread(com->print_buff,"%s: thread mk cli start\n", CMD); 
  



  if(com->sock == -1)
  {
    create_client(com);
    if(com->pthread_rcv == NULL) 
    {
      sprintf_fifo_thread(com->print_buff,"%s: create rcv thread\n", CMD);
      pthread_create(&com->pthread_rcv, NULL, &thread_rcv_f, com); 
    }
    if(com->pthread_send == NULL) 
    {
      sprintf_fifo_thread(com->print_buff,"%s: create send tread\n", CMD);
      pthread_create(&com->pthread_send, NULL, &thread_send_f, com);
    }
  }
  else
  {
    sprintf_fifo_thread(com->print_buff,"%s: thread cli started by serveur, with socket : %d\n", CMD, com->sock);
  }

  while (com->running)
  {
    usleep(100);
  }
  
  if (close(com->sock) == -1)
    erreur_IO("close");
  
  usleep(100000);
  //run = 0;
}

void* thread_mk_serv(void *arg)
{
  com_struct_t* com = (com_struct_t*) arg;
  int nb_cli = 0;
  int nb_cli_max = 3;
  com_struct_t cli_tab[nb_cli_max];


  char CMD[] ="mk_serv";
  sprintf_fifo_thread(com->print_buff,"%s: thread mk serv start\n", CMD);
  
  int ecoute = create_socket(com->print_buff, com->local_port_str);



  
  while (com->running)
  {
    if(nb_cli < nb_cli_max)
    {
      cli_tab[nb_cli] = *com;

      struct sockaddr_in adrClient;
      unsigned int lgAdrClient;
      lgAdrClient = sizeof(adrClient);
      cli_tab[nb_cli].sock = accept(ecoute, (struct sockaddr *)&adrClient, &lgAdrClient);
      if (cli_tab[nb_cli].sock < 0)
        erreur_IO("accept");
      // TODO get client adress an port info
      sprintf_fifo_thread(cli_tab[nb_cli].print_buff,"serveur maker: connected to : adr %s, listen_port %d\n",
            stringIP(ntohl(adrClient.sin_addr.s_addr)),
            ntohs(adrClient.sin_port));



      if(cli_tab[nb_cli].pthread_send == NULL)
      {
        sprintf_fifo_thread(cli_tab[nb_cli].print_buff,"%s: create send thread\n", CMD); 
        pthread_create(&cli_tab[nb_cli].pthread_send, NULL, &thread_send_f, &cli_tab[nb_cli]);
      }
      if(cli_tab[nb_cli].pthread_rcv == NULL)
      {
        sprintf_fifo_thread(cli_tab[nb_cli].print_buff,"%s: create rcv tread\n", CMD); 
        pthread_create(&cli_tab[nb_cli].pthread_rcv, NULL, &thread_rcv_f, &cli_tab[nb_cli]);
      }

      nb_cli ++;
    }
    else usleep(100);
  }
  
  for(int i = 0; i< nb_cli; i++)
    if (close(cli_tab[i].sock) == -1)
      erreur_IO("close canal");

  if(ecoute != -1)
    if (close(ecoute) == -1)
      erreur_IO("close ecoute");
  
  usleep(100000);
  //run = 0;

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

void* thread_add_fifo_input_f(void *arg)
{
  term_var_struct_t* term_var = (term_var_struct_t*)arg;
  char line_to_input[PRINT_LINE_MAX];
  int car_id = 0;

  while(run)
  {
    if(line_to_input[car_id] = getc(stdin))
    {
      car_id++;
      if(car_id == PRINT_LINE_MAX-1 || line_to_input[car_id-1] == '\n')
      {
        line_to_input[car_id] = '\0';
        add_fifo_thread_sub(term_var->input_buff, line_to_input);
        sprintf_fifo_thread(term_var->print_buff,"");// renew username at the begining of the line
        car_id = 0;
      }
    }
  }
}

void* thread_pop_fifo_log_f(void *arg)
{
  fifo_thread_t* fifo_log = (fifo_thread_t*)arg;
  char line_to_log[PRINT_LINE_MAX];
  log_file = open(log_file_str,O_CREAT|O_WRONLY|O_APPEND, 0644); // oppen log file

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

 // thread send messages
void* thread_send_f(void* arg)
{
  com_struct_t* com = (com_struct_t*) arg; 
  
  char CMD[] ="send";
  sprintf_fifo_thread(com->print_buff,"%s: thread send start\n", CMD); 


  ssize_t nbRead, nbRead_tot = 0;

  char ligne[LIGNE_MAX];
  int lgEcr;
  int pop_status = -1;

  int input_buff_sub = add_sub_fifo_thread_sub(com->input_buff);

while( com->running && (input_buff_sub != -1) )
{
  if(pop_status == -1);
  else if(pop_status == 0)  
  {
    sprintf_fifo_thread(com->print_buff,"arret par CTRL-D\n");
  }
  else
  {
    lgEcr = ecrireLigne(com->sock, ligne); // send the message
    sprintf_fifo_thread(com->log_buff,"<%s>:%s", pseudo_local, ligne); // log the message

    if (lgEcr == -1)
      erreur_IO("ecrireLigne");

    nbRead = strlen(ligne); 
    nbRead_tot += nbRead;

    int res_regex = check_cmd(ligne);

    if(res_regex & END_TRANSMIT)
    {
      com->running = 0;
      sprintf_fifo_thread(com->print_buff,"%s: End sended, close serveur\n", CMD);
    }
    if(res_regex & CLEAR_TRANSMIT)
    {
      sprintf_fifo_thread(com->print_buff,"\nclean\n");
    }

  }
  pop_status = pop_fifo_thread_sub(com->input_buff, ligne, input_buff_sub);
}

  
  
  sprintf_fifo_thread(com->print_buff,"%s: %ld char have been transfered\n",CMD, nbRead_tot);
  return NULL;
}


// thread receive messages
void* thread_rcv_f(void* arg)
{
  
  com_struct_t* com = (com_struct_t*) arg;
  

  char CMD[] ="rcv";
  sprintf_fifo_thread(com->print_buff,"%s: thread rcv start\n", CMD);
  
  ssize_t lgLue, lgLue_tot = 0;


  char ligne[LIGNE_MAX];

 

  while (com->running)
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
        com->running = 0;
        sprintf_fifo_thread(com->print_buff,"%s: End recieved, close serveur\n", CMD);
      }
      if(res_regex & CLEAR_TRANSMIT)
      {
        sprintf_fifo_thread(com->print_buff,"\nclean\n");
      }

    }
  }


  //////////////////
  
  sprintf_fifo_thread(com->print_buff,"%s: %ld char have been transfered\n",CMD, lgLue_tot);
  return NULL;
}

//c ckeck if the command is a valid command
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

// print onto terminal
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

// create a socket and bind it to a server
int connect_socket(fifo_thread_t* fifo_print, const char* addr_str, const char* port_str)
{
  char CMD[] ="client: socket";
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

// create a socket and open a server
int create_socket(fifo_thread_t* fifo_print, const char *port_str)
{
  char CMD[] ="serveur: socket";

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


// connect to a server
void create_client( com_struct_t* cli)
{

  cli->sock = connect_socket(cli->print_buff, cli->dist_addr_str, cli->dist_port_str);


  sprintf_fifo_thread(cli->print_buff,"client maker: connected to : adr %s, port %s\n",
        cli->dist_addr_str,
        cli->dist_port_str);


}

// create a server, blocking while waiting for a client
int create_serveur(com_struct_t* serv)
{

  int ecoute = create_socket(serv->print_buff, serv->local_port_str);

  struct sockaddr_in adrClient;
  unsigned int lgAdrClient;
  lgAdrClient = sizeof(adrClient);
  serv->sock = accept(ecoute, (struct sockaddr *)&adrClient, &lgAdrClient);
  if (serv->sock < 0)
    erreur_IO("accept");
 // TODO get client adress an port info
  sprintf_fifo_thread(serv->print_buff,"serveur maker: connected to : adr %s, listen_port %d\n",
        stringIP(ntohl(adrClient.sin_addr.s_addr)),
        ntohs(adrClient.sin_port));
  return ecoute;
}
