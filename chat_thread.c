/* programme (c) 
author : Colle Chaude
*/

#include <stdio.h>
#include <stdarg.h>
#include "pse.h"
#include "fifo_thread_lib.h"
#include "fifo_thread_sub_lib.h"
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

  int sock;
  int running;
  pthread_t pthread_rcv, pthread_send;

}com_struct_t;

typedef struct
{
  fifo_thread_t* print_buff;
  fifo_thread_sub_t* input_buff;
  fifo_thread_t* log_buff;

  char listen_port_str[8];
  int sock;
  int running;

  int nb_cli;
  int nb_cli_max;
  com_struct_t* connected_com;
}server_struct_t;

typedef struct 
{
  fifo_thread_t* print_buff;
  fifo_thread_sub_t* input_buff;
  fifo_thread_t* log_buff;
  char dist_addr_str[30];
  char dist_port_str[8];

  com_struct_t connected_com;
}client_struct_t;


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


int connect_socket(fifo_thread_t* fifo_print, const char* addr_str, const char* port_str);
int create_socket(fifo_thread_t* fifo_print, const char *port_str, const int nb_max_client);


char username[30] =   "local--";
char log_file_str[60] = "journal.log";
int log_file;

int run = 1; 
int absolute_end = 0;

int main(int argc, char *argv[]) {
  int is_client = 0;
  int is_server = 0;
  run = 1; 


  pthread_t pthread_fifo_print, pthread_fifo_input, pthread_fifo_log;
  fifo_thread_t fifo_print; // fifo for print in the terminal
  fifo_thread_sub_t fifo_input; // fifo for the input (stdin)
  fifo_thread_t fifo_log; // fifo for thpthread_fifo_inpute log file

  
  init_fifo_thread(&fifo_log, NB_LINE_PRINT, PRINT_LINE_MAX); // initialisation de la fifo de log
  init_fifo_thread(&fifo_print, NB_LINE_PRINT, PRINT_LINE_MAX); // initialisation de la fifo de print
  init_fifo_thread_sub(&fifo_input, NB_LINE_PRINT, PRINT_LINE_MAX, 5); // initialisation de la fifo de input

  pthread_t mk_cli_thread_handler, mk_serv_thread_handler;
  server_struct_t server_s = {&fifo_print, &fifo_input, &fifo_log, "2000", -1, 1, 0, 10};
  client_struct_t client_s = {&fifo_print, &fifo_input, &fifo_log, "localhost", "2000", {&fifo_print, &fifo_input, &fifo_log, -1, 1}};


  //com_struct_t com_s = {&fifo_print, &fifo_input, &fifo_log, -1, 1}; // initialisation structure du client
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
        if(i < argc-1)strcpy(client_s.dist_addr_str, argv[i+1]);
        break;
      case 'p':
        if(i < argc-1)strcpy(client_s.dist_port_str, argv[i+1]);
        break;
      case 'l':
        if(i < argc-1)strcpy(server_s.listen_port_str, argv[i+1]);
        break;
      case 'u':
        if(i < argc-1)strcpy(username, argv[i+1]);
        break;
      case 'o':
        if(i < argc-1)strcpy(log_file_str, argv[i+1]);
        break;
      case 'c':
        is_client = 1;
        break;
      case 's':
        is_server = 1;
        break;
      case 'h':
        printf("%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n","Help : ",
          "-a : addresse de l'interlocuteur",
          "-p : port de l'interlocuteur",
          "-l : port d'ecoute local",
          "-u : mon username",
          "-o : fichier de log",
          "-s : mode serveur",
          "-c : mode client");
        run = 0;
        break;
      
      default:
        break;
      }
    }

  }


  if(run)
  {

    // ----------  creating threads  -------

    pthread_create(&pthread_fifo_print, NULL, &thread_pop_fifo_print_f, &fifo_print);
    pthread_create(&pthread_fifo_input, NULL, &thread_add_fifo_input_f, &term_var_s);
    pthread_create(&pthread_fifo_log, NULL, &thread_pop_fifo_log_f, &fifo_log);

    //  create client or/and server
    printf("creating thread\n");
    if(is_client)
    {
      printf("creating client\n");
      pthread_create(&mk_cli_thread_handler, NULL, &thread_mk_cli, &client_s);
      
    }
    if(is_server)
    {
      printf("creating server\n");
      pthread_create(&mk_serv_thread_handler, NULL, &thread_mk_serv, &server_s);
    }
    printf("thread created\n");

    if(is_client) pthread_join(mk_cli_thread_handler, NULL);
    if(is_server) pthread_join(mk_serv_thread_handler, NULL);
    

    run = 0;
    printf("Closing program\n");
    printf("Closing fifo_print\n");
    pthread_join(pthread_fifo_print, NULL);
    printf("Closing fifo_input\n");
    pthread_join(pthread_fifo_input, NULL);
    printf("Closing fifo_log\n");
    pthread_join(pthread_fifo_log, NULL);
    printf("thread closed\n\n");
    return EXIT_SUCCESS;
  }

}

// make a client, connect to the server and start thread to send and recieve messages
void* thread_mk_cli(void *arg)
{
  client_struct_t* cli_s = (client_struct_t*) arg; 
  
  char CMD[] ="mk_cli";
  sprintf_fifo_thread(cli_s->print_buff,"%s: thread mk cli start\n", CMD); 
  
  com_struct_t* com = &cli_s->connected_com;

  if(com->sock == -1)
  {
    com->sock = connect_socket(cli_s->print_buff, cli_s->dist_addr_str, cli_s->dist_port_str);

    sprintf_fifo_thread(cli_s->print_buff,"client maker: connected to : adr %s, port %s\n",
          cli_s->dist_addr_str,
          cli_s->dist_port_str);

    if(com->pthread_rcv == NULL) 
    {
      sprintf_fifo_thread(cli_s->print_buff,"%s: create rcv thread\n", CMD);
      pthread_create(&com->pthread_rcv, NULL, &thread_rcv_f, com); 
    }
    if(com->pthread_send == NULL) 
    {
      sprintf_fifo_thread(cli_s->print_buff,"%s: create send tread\n", CMD);
      pthread_create(&com->pthread_send, NULL, &thread_send_f, com);
    }
  }
  else
  {
    sprintf_fifo_thread(cli_s->print_buff,"%s: thread cli started by server, with socket : %d\n", CMD, com->sock);
  }

  while (com->running)
  {
    usleep(100);
  }
  
  sprintf_fifo_thread(cli_s->print_buff,"%s: thread mk cli ending\n", CMD);

  pthread_join(com->pthread_rcv, NULL);
  pthread_join(com->pthread_send, NULL);

  if (close(com->sock) == -1)
    erreur_pthread_IO("close");
  
  usleep(100000);

  sprintf_fifo_thread(cli_s->print_buff,"%s: thread mk cli ended\n", CMD);
  //run = 0;

	pthread_exit(EXIT_SUCCESS);
}

// make a server, listen to the client and start thread to send and recieve messages
void* thread_mk_serv(void *arg)
{
  server_struct_t* serv_s = (server_struct_t*) arg;
  serv_s->nb_cli = 0;

  serv_s->connected_com = malloc(sizeof(com_struct_t) * serv_s->nb_cli_max);

  char CMD[] ="mk_serv";
  sprintf_fifo_thread(serv_s->print_buff,"%s: thread mk serv start\n", CMD);
  
  serv_s->sock = create_socket(serv_s->print_buff, serv_s->listen_port_str, serv_s->nb_cli_max);
  fcntl(serv_s->sock, F_SETFL, O_NONBLOCK);


  while (serv_s->running && !absolute_end)
  {
    if(serv_s->nb_cli < serv_s->nb_cli_max)
    {
      com_struct_t* com = &serv_s->connected_com[serv_s->nb_cli];
      com->print_buff = serv_s->print_buff;
      com->input_buff = serv_s->input_buff;
      com->log_buff = serv_s->log_buff;
      com->running = 1;


      struct sockaddr_in adrClient;
      unsigned int lgAdrClient;
      lgAdrClient = sizeof(adrClient);

      while (serv_s->running && !absolute_end)
      {
        com->sock = accept(serv_s->sock, (struct sockaddr *) &adrClient, &lgAdrClient);
        if (com->sock != -1)
        {
          break;
        }
        usleep(100);
      }
      //com->sock = accept(serv_s->sock, (struct sockaddr *)&adrClient, &lgAdrClient);
      
      if (com->sock >= 0)
      {
        // TODO get client adress an port info
        sprintf_fifo_thread(com->print_buff,"server maker: connected to : adr %s, listen_port %d\n",
              stringIP(ntohl(adrClient.sin_addr.s_addr)),
              ntohs(adrClient.sin_port));

        if(com->pthread_send == NULL)
        {
          sprintf_fifo_thread(com->print_buff,"%s: create send thread\n", CMD); 
          pthread_create(&com->pthread_send, NULL, &thread_send_f, com);
        }
        if(com->pthread_rcv == NULL)
        {
          sprintf_fifo_thread(com->print_buff,"%s: create rcv tread\n", CMD); 
          pthread_create(&com->pthread_rcv, NULL, &thread_rcv_f, com);
        }

        serv_s->nb_cli ++;
      }
    }
    else usleep(100);
  }
  printf("absolute end\n");

  sprintf_fifo_thread(serv_s->print_buff,"%s: thread mk serv ending\n", CMD);
  for(int i = 0; i< serv_s->nb_cli; i++)
  {
    pthread_join(serv_s->connected_com[i].pthread_send, NULL);
    pthread_join(serv_s->connected_com[i].pthread_rcv, NULL);
    if (close(serv_s->connected_com[i].sock) == -1)
      erreur_pthread_IO("close canal");
  }

  if(serv_s->sock != -1)
    if (close(serv_s->sock) == -1)
      erreur_pthread_IO("close socket");

  free(serv_s->connected_com);

  sprintf_fifo_thread(serv_s->print_buff,"%s: thread mk serv ended\n", CMD);
  usleep(100000);
  //run = 0;

	pthread_exit(EXIT_SUCCESS);
}

void* thread_pop_fifo_print_f(void *arg)
{
  fifo_thread_t* fifo_print = (fifo_thread_t*)arg;
  char line_to_print[PRINT_LINE_MAX];
  while(run)
  {
    if(try_pop_fifo_thread(fifo_print, line_to_print))
      printf_chat(username, line_to_print);
    else usleep(100);
  }

  pthread_exit(EXIT_SUCCESS);
}

void* thread_add_fifo_input_f(void *arg)
{
  term_var_struct_t* term_var = (term_var_struct_t*)arg;
  char line_to_input[PRINT_LINE_MAX];
  int car_id = 0;
  

  while(run)
  {
    // TODO add a way to stop the thread, use a non blocking getc
    if(line_to_input[car_id] = getc(stdin))
    {
      car_id++;
      if(car_id == PRINT_LINE_MAX-1 || line_to_input[car_id-1] == '\n')
      {
        line_to_input[car_id] = '\0';
        sprintf_fifo_thread_sub(term_var->input_buff,"<%s>:%s", username, line_to_input);
        sprintf_fifo_thread(term_var->print_buff,"");// renew username at the begining of the line
        car_id = 0;


        int res = check_cmd(line_to_input);
        if(res & END_TRANSMIT)
        {
          absolute_end = 1;
          sprintf_fifo_thread_sub(term_var->input_buff,"input manager: absolute end\n");
        }
      }
    }
    else usleep(100);
  }

  pthread_exit(EXIT_SUCCESS);
}

void* thread_pop_fifo_log_f(void *arg)
{
  fifo_thread_t* fifo_log = (fifo_thread_t*)arg;
  char line_to_log[PRINT_LINE_MAX];
  log_file = open(log_file_str,O_CREAT|O_WRONLY|O_APPEND, 0644); // oppen log file

  if (log_file == -1) {
    char errmessage[90];
    sprintf(errmessage, "open output, unable to open \"%s\"", log_file_str);
    erreur_pthread_IO(errmessage);
  }
  else
  {
    while(run)
    {
      if(try_pop_fifo_thread(fifo_log, line_to_log))
        write(log_file, line_to_log, strlen(line_to_log));
      else usleep(100);
    }
  }

  if (close(log_file) == -1) {
    erreur_pthread_IO("close output");
  }
  
  pthread_exit(EXIT_SUCCESS);
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
    sprintf_fifo_thread(com->log_buff, ligne);
    //sprintf_fifo_thread(com->log_buff,"<%s>:%s", username, ligne); // log the message

    if (lgEcr == -1)
      erreur_pthread_IO("ecrireLigne");

    nbRead = strlen(ligne); 
    nbRead_tot += nbRead;

    int res_regex = check_cmd(ligne);

    if(res_regex & END_TRANSMIT)
    {
      com->running = 0;
      sprintf_fifo_thread(com->print_buff,"%s: End sended, close server\n", CMD);
    }
    if(res_regex & CLEAR_TRANSMIT)
    {
      sprintf_fifo_thread(com->print_buff,"\nclean\n");
    }

  }
  pop_status = pop_fifo_thread_sub(com->input_buff, ligne, input_buff_sub);
}

  
  
  sprintf_fifo_thread(com->print_buff,"%s: %ld char have been transfered\n",CMD, nbRead_tot);
  
  

	pthread_exit(EXIT_SUCCESS);
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
      
      //sprintf_fifo_thread(com->print_buff,"<%s>:%s", com->pseudo_distant, ligne);
      //sprintf_fifo_thread(com->log_buff,"<%s>:%s", com->pseudo_distant, ligne);
      sprintf_fifo_thread(com->print_buff, ligne);
      sprintf_fifo_thread(com->log_buff, ligne);

      int res_regex = check_cmd(ligne);

      if(res_regex & END_TRANSMIT)
      {
        com->running = 0;
        sprintf_fifo_thread(com->print_buff,"%s: End recieved, close server\n", CMD);
      }
      if(res_regex & CLEAR_TRANSMIT)
      {
        sprintf_fifo_thread(com->print_buff,"\nclean\n");
      }

    }
  }


  //////////////////
  
  sprintf_fifo_thread(com->print_buff,"%s: %ld char have been transfered\n",CMD, lgLue_tot);
  
  
	pthread_exit(EXIT_SUCCESS);
}

//c ckeck if the command is a valid command
int check_cmd(char* line)
{
  int ret = 0;
  char abs_fin_ex_str[] = "^fin\n?$";
  char fin_ex_str[] = ":fin\n?$";
  char init_ex_str[] = ":init\n?$";
  regex_t abs_fin_ex;
  regex_t fin_ex;
  regex_t init_ex;
  regcomp(&abs_fin_ex, abs_fin_ex_str, REG_EXTENDED|REG_NOSUB);
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
  if (regexec(&abs_fin_ex, line, 0, NULL, 0) == 0)
  {
    ret |= END_TRANSMIT;
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
    erreur_pthread_IO("socket");

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
    erreur_pthread_IO("connect");
  sprintf_fifo_thread(fifo_print,"%s:.. connected\n", CMD);
  return sock;
}

// create a socket and open a server
int create_socket(fifo_thread_t* fifo_print, const char *port_str, const int nb_max_client)
{
  char CMD[] ="server: socket";

  int sock, ret;
  struct sockaddr_in adrsock;
  short listen_port = (short)atoi(port_str);

  sprintf_fifo_thread(fifo_print,"%s: creating a socket\n", CMD);
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    erreur_pthread_IO("socket");
  
  adrsock.sin_family = AF_INET;
  adrsock.sin_addr.s_addr = INADDR_ANY;
  adrsock.sin_port = htons(listen_port);
  sprintf_fifo_thread(fifo_print,"%s: binding to INADDR_ANY address on port %d\n", CMD, listen_port);
  ret = bind (sock,  (struct sockaddr *)&adrsock, sizeof(adrsock));
  if (ret < 0)
    erreur_pthread_IO("bind");
  
  sprintf_fifo_thread(fifo_print,"%s: listening to socket\n", CMD);
  ret = listen (sock, nb_max_client);
  if (ret < 0)
    erreur_pthread_IO("listen");
  
  sprintf_fifo_thread(fifo_print,"%s: accepting a connection\n", CMD);
  
  return sock;
}

