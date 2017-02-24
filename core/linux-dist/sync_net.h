/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   sync_net.h
 * Author: programmeur
 *
 * Created on 16 февраля 2017 г., 23:17
 */

//#ifndef SYNC_NET_H
//#define SYNC_NET_H


struct DCPadKeymap
{
    unsigned short int  DPad[4];  //крестовина
    unsigned short int  SPad[4];  //мягкий джой  4-е зарегестрировано для клавиатуры (на всякий) 
    unsigned short int  Buttons[5];  //кнопки
    unsigned short int  Triggers[2];  //триггеры
    bool InvertX, InvertY; 
    
    bool plugIn;   //подключить устройство?
    bool PlugMMU[2];  //подключить карты памяти?
    
    std::string MemoryCardPath[2];  //путь к файлу карты памяти
    char DeviceEvNumber;  //num of dev
    
};

 typedef struct DCPadKeymap  DCPads[4];  //у нас до 4-х джоев 
 


char CreateSock(void);
bool ConfigurePADS(DCPads &CfgPads);
void ProcessMessagesFromServer(void);
void* wrk_sock_thread(void * arg);
bool LoadEmuConfig(void);

//#endif /* SYNC_NET_H */

