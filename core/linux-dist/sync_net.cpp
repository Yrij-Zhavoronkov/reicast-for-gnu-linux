/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <pthread.h>
#include <strings.h>

#include "sync_net.h"
#include "cfg/cfg.h"
#include <unistd.h>

//#include <sync_net.h> //мой хедер

int IntSock=-1;
typedef char  TIO_bufer[4096];

using namespace std;

char CreateSock(void)
{
    //define
    struct sockaddr_in SAddr;  
  
   
    if ((IntSock = socket(AF_INET, SOCK_STREAM, 0))<0)
    {
        cout <<"Сокет не создан!";
        return(10);   //error 10
    };
    
    // Указываем параметры сервера
    SAddr.sin_family = AF_INET;
    SAddr.sin_port = htons(22409);
    SAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    
    if (connect(IntSock, (struct sockaddr *)&SAddr, sizeof(SAddr)) < 0) // установка соединения с сервером
    {
         cout <<"Ошибка подключени к серверу";
        return(11);
    };
       
    //отправим версию, чтоб знал
    string message="reicast_core=20170218";
    char in_buf[512];
    
    send(IntSock,  message.data(), message.size(), 0); // отправка сообщения на сервер
    message="";
    
    
    int Read_ct = 0;
    std::cout <<"Жду ответа сервера\n";
    
    Read_ct = recv(IntSock, in_buf, 512, 0);
    
     std::cout <<"ответ сервера='"<<in_buf<<"'\n";
    
    if (((string) in_buf)=="done!") 
        {return 0;} 
    else 
        {return 13;}
};


/*вернет строку из буфера байт*/
string GetStringViaBuf(TIO_bufer &buf, unsigned int &position)
{
    string StringBuf="";
    
    while ((position<sizeof(buf)) && (buf[position]!=0))
    {
       StringBuf+=buf[position];
       position++;        
    };
    
    //если мы считали что-либо то сдвинь с позиции нуля!
    if (position+1<sizeof(buf)) position++;
    
    return StringBuf;
};


/*Эта процедура должна быть вызвана только после загрузки эмулятором основного конфига
 все получаемые по сети параметры заменят параметры конфига
 управление будет в отдельной структуре
 */
bool ConfigurePADS(DCPads &CfgPads)   //получим параметры эмулятора
{
        //шлем запрос на сервер!
        string message="GET_PADS_DATA";
        TIO_bufer in_buf;   
     
    
        //std::cout <<">>GET_PADS_DATA\n";
     
        send(IntSock,  message.data(), message.size(), 0); // отправка сообщения на сервер
        message="";    
    
        unsigned int Read_ct = 0;
        unsigned int read_pos=0, cur_pad=0, cur_pos;
        
        
        std::cout <<"Жду ответа сервера\n";
    
        Read_ct = recv(IntSock, in_buf, sizeof(in_buf), 0);     
     
        /*ПОКА НЕ прочтем все*/ 
        while ((read_pos<Read_ct) && (cur_pad<4))
        {
             //std::cout <<"PAD="<<cur_pad<<endl<<endl;
            //загрузка крестовин
            for (cur_pos=0; cur_pos<4; cur_pos++)
            {
               
                CfgPads[cur_pad].DPad[cur_pos]=(in_buf[read_pos]*256+in_buf[read_pos+1] );
               // std::cout <<"Крестовина="<<CfgPads[cur_pad].DPad[cur_pos]<<endl;
                read_pos+=2; 
            };
        
            //мягкий джой
            for (cur_pos=0; cur_pos<4; cur_pos++)
            {
                   CfgPads[cur_pad].SPad[cur_pos]=(in_buf[read_pos]*256+in_buf[read_pos+1] );
                  // std::cout <<"Мягкий джой="<<CfgPads[cur_pad].SPad[cur_pos]<<endl;
                   read_pos+=2; 
            };
        
            //кнопки
            for (cur_pos=0; cur_pos<5; cur_pos++)
            {
                CfgPads[cur_pad].Buttons[cur_pos]=(in_buf[read_pos]*256+in_buf[read_pos+1]);
                //std::cout <<"Кнопка="<<CfgPads[cur_pad].Buttons[cur_pos]<<endl;
                read_pos+=2; 
            };
        
        //триггеры
        for (cur_pos=0; cur_pos<2; cur_pos++)
        {
           CfgPads[cur_pad].Triggers[cur_pos]=(in_buf[read_pos]*256+in_buf[read_pos+1] );
          // std::cout <<"Тригер="<<CfgPads[cur_pad].Triggers[cur_pos]<<endl;
           read_pos+=2; 
        };
        
        //Инверт
        CfgPads[cur_pad].InvertX=in_buf[read_pos];
       // std::cout <<"ИнвертХ="<<CfgPads[cur_pad].InvertX<<endl;
        CfgPads[cur_pad].InvertY=in_buf[read_pos+1];
       //std::cout <<"ИнвертУ="<<CfgPads[cur_pad].InvertY<<endl;
        
        //подключить джой?
        CfgPads[cur_pad].plugIn=in_buf[read_pos+2];
       // std::cout <<"Джой подключен="<<CfgPads[cur_pad].plugIn<<endl;
        
         read_pos+=3; 
        
        /*дальше идут стринги  читать до нуля или же положить в первый байт длинну?
         .....читаем до \0 */ 
        
         CfgPads[cur_pad].MemoryCardPath[0]=GetStringViaBuf(in_buf, read_pos);
        // std::cout <<"Путь MMU='"<<CfgPads[cur_pad].MemoryCardPath[0]<<"'\n";
         CfgPads[cur_pad].MemoryCardPath[1]=GetStringViaBuf(in_buf, read_pos);
        // std::cout <<"Путь MMU='"<<CfgPads[cur_pad].MemoryCardPath[1]<<"'\n";
        
         CfgPads[cur_pad].DeviceEvNumber             =in_buf[read_pos];
        // std::cout <<"DEV name='"<<CfgPads[cur_pad].DeviceEvNumber<<"'\n";
         read_pos++; 
         
                  
          //подключить карты памяти?
        CfgPads[cur_pad].PlugMMU[0]=(CfgPads[cur_pad].MemoryCardPath[0]!="");
        CfgPads[cur_pad].PlugMMU[1]=(CfgPads[cur_pad].MemoryCardPath[1]!="");
         
         //один пульт загрузили! Цикл!       
          
           
                
        /* if (CfgPads[cur_pad].DeviceEvNumber>1) //те уже реальное устройство
             MycfgSaveInt("input",  branch, CfgPads[cur_pad].DeviceEvNumber);
         else 
             MycfgSaveInt("input",branch, -1);  */ 
        
         // std::cout <<"END PAD="<<cur_pad<<endl;
          cur_pad++;
          
          
          if (Read_ct<read_pos) return false;
     };
     
     return true;
};

bool LoadEmuConfig(void)
{
        //шлем запрос на сервер!
        string message="GET_CONFIG_DATA";
        char in_buf[128];
        extern bool x11_fullscreen;
        extern int   x11_width; 
        extern int   x11_height;
    
        //std::cout <<">>"<<message<<"\n";
     
        send(IntSock,  message.data(), message.size(), 0); // отправка сообщения на сервер
        message="";    
    
        
        std::cout <<"Жду ответа сервера\n";
    
        if (recv(IntSock, in_buf, sizeof(in_buf), 0) != 48) return false;
          
        /*пишем конфиг*/
        x11_fullscreen=((in_buf[0] | in_buf[1])==0);   //объявлен в x11.cpp 
        x11_width= in_buf[0]*256+in_buf[1];
        x11_height=in_buf[2]*256+in_buf[3];
        
        settings.rend.UseMipmaps=in_buf[4];
        settings.rend.WideScreen=in_buf[5];
        settings.pvr.MaxThreads=in_buf[6];
        settings.pvr.SynchronousRendering= in_buf[7];
        settings.validate.OpenGlChecks=in_buf[8];
        /*SND*/
         settings.aica.LimitFPS=in_buf[16];  
         settings.aica.NoSound=in_buf[17];
         settings.aica.DSPEnabled=in_buf[18];
         settings.aica.GlobalFocus=in_buf[19];
         settings.aica.BufferSize=in_buf[20]*256+in_buf[21];
         /*SYS*/
         settings.dynarec.Enable=in_buf[36];			
        settings.dynarec.idleskip=in_buf[37];		
        settings.dynarec.unstable_opt=in_buf[38];                         
        
        return true;
}


void ProcessMessagesFromServer(void)  //обработка дальнейших команд сервера..
{

};

void* wrk_sock_thread(void * arg) //поток обсл сокет
{   
   if (IntSock!=-1)
      {     
        //слушаем сокет дальше (для команд стоп, старт, свернуть, выгрузка, загрузака и пр)
        ProcessMessagesFromServer();  
      }; 
};

void SetupExecIso(void)
{
        string message="GET_ISO_NAME";
            
        send(IntSock,  message.data(), message.size(), 0); // отправка сообщения на сервер
        message="";    
            
        std::cout <<"Жду ответа сервера\n";        
               
        recv(IntSock, settings.imgread.DefaultImage, sizeof(settings.imgread.DefaultImage), 0);
        
        //?? РЕШЕНИЕ, запускать или нет?
        settings.imgread.LoadDefaultImage=(access(settings.imgread.DefaultImage, 0)==0);
}