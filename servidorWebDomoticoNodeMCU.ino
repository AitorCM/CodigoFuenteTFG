/*Librerias*/
#include <ESP8266WiFi.h>
#include <Servo.h>

/*Asignacion de pines de Arduino a los pines GPIO de NodeMCU LoLin*/
#define A0 A0       //Sensor Temperatura        

#define D0 16       //Luces Entrada
#define D1 5        //Luces Cocina
#define D2 4        //Luz Aire Acondicionado
#define D3 0        //Trigger Sensor Distancia Garaje
#define D10 1       //Echo Sensor Distancia Garaje
#define D4 2        //Motor Puerta Entrada
#define D5 14       //Motor Ventilador
#define D6 12       //Motor Persiana Abajo
#define D7 13       //Motor Persiana Arriba
#define D8 15       //Sensor Magnetico Puerta Principal
#define D9 3        //Motor Puerta Garaje




//Conectar NodeMCU a red Wi-Fi
const char* ssid = "ssid";
const char* password = "password";


//Declaracion Pines Luces
int pinLedEntrada = 16;                            //GPIO 16 Led Entrada
int pinLedCocina = 5;                              //GPIO 05 Led Cocina Comedor
   

//Declaracion Pin Aire Acondicionado
int pinLedAC = 4;                                  //GPIO 04 Led Habitacion

//Declaracion Pines Motores
int pinMotorGaraje = 3;                            //GPIO 00 Motor Cochera
int pinMotorEntrada = 2;                           //GPIO 02 Motor Puerta Entrada
int pinMotorVentilador = 14;                       //GPIO 14 Motor Ventilador
int pinMotorPersianasAbajo = 12;                   //GPIO 12 Motor Persianas Abajo
int pinMotorPersianasArriba = 13;                  //GPIO 13 Motor Persianas Arriba


//Declaracion Pines Sensores
int pinSensorPuertaPrincipal = 15;                 //GPIO 15 Sensor Magnetico Puerta Principal
int triggerPinSensorCochera = 0;                   //GPIO 03 Trigger Sensor Distancia Puerta Cochera
int echoPinSensorCochera = 1;                      //GPIO 01 Echo Sensor Distancia Puerta Cochera
int pinSensorTemperatura = A0;                     //A0 Sensor Temperatura



/*DECLARACION OBJETOS */
Servo servoGaraje;
Servo servoEntrada;
Servo servoPersianaPisoAbajo;
Servo servoPersianaPisoArriba;
Servo servoVentilador;


int posicionServoGaraje;
int posicionServoEntrada;
int posicionServoPersianaPisoAbajo;
int posicionServoPersianaPisoArriba;
int posicionServoVentilador;


//Declaracion del puerto 80 como puerto del servidor
WiFiServer server(80);


void setup(){
  
 Serial.begin(115200);
 delay(10);

 servoGaraje.attach(pinMotorGaraje);
 servoEntrada.attach(pinMotorEntrada);
 servoPersianaPisoAbajo.attach(pinMotorPersianasAbajo);
 servoPersianaPisoArriba.attach(pinMotorPersianasArriba);
 servoVentilador.attach(pinMotorVentilador);

 posicionServoGaraje = 0;
 posicionServoEntrada = 0;
 posicionServoPersianaPisoAbajo = 0;
 posicionServoPersianaPisoArriba = 0;
 posicionServoVentilador = 90;    //Detenido
 
 servoGaraje.write(posicionServoGaraje);
 servoEntrada.write(posicionServoEntrada);
 servoPersianaPisoAbajo.write(posicionServoPersianaPisoAbajo);
 servoPersianaPisoArriba.write(posicionServoPersianaPisoArriba);
 servoVentilador.write(posicionServoVentilador);

 /*PinMode del sensor de TEMPERATURA*/
 pinMode(pinSensorTemperatura, INPUT);

 /*PinMode del sensor MAGNETICO de la PUERTA PRINCIPAL*/
 pinMode(pinSensorPuertaPrincipal, INPUT);

 /*PinMode del sensor de DISTANCIA de la puerta GARAJE*/
 pinMode(triggerPinSensorCochera, OUTPUT);
 pinMode(echoPinSensorCochera, INPUT);  
 
 /*PinMode de los LED*/
 pinMode(pinLedEntrada, OUTPUT);
 pinMode(pinLedCocina, OUTPUT);

 digitalWrite(pinLedEntrada,LOW);
 digitalWrite(pinLedCocina,LOW);

 /*PinMode del Led del Aire Acondicionado*/
 pinMode(pinLedAC, OUTPUT);
 digitalWrite(pinLedAC,LOW);

 //Conectar al WiFi
 Serial.println();
 Serial.println();
 Serial.print("Conectando a ");
 Serial.println(ssid);

 WiFi.begin(ssid,password);

 while(WiFi.status() != WL_CONNECTED){
  delay(500);
  Serial.print(".");
 }

 Serial.println("");
 Serial.println("WiFi Connected");

 //Iniciar el Servidor
 server.begin();
 Serial.println("Servidor Iniciado");

 //Printear la direccion IP
 Serial.print("Usa esta URL para conectarte: ");
 Serial.print("http://");
 Serial.print(WiFi.localIP());
 Serial.println("/");
 
}//Final setup


void loop(){

  /*LECTURA DE PINES DE ENTRADA/SALIDA*/

  /*Lectura Pines LED*/
  int valorLedCocina = digitalRead(pinLedCocina);  
  int valorLedEntrada = digitalRead(pinLedEntrada);
  

  /*Lectura Pin Aire Acondicionado*/
  int valorLedAC = digitalRead(pinLedAC);
  
  
  /*Lectura Pin Termometro*/
  float lectura = analogRead(pinSensorTemperatura);
  //float millivolts = (lectura/1023.0)*5000;
  float celsius = (3.3* lectura *100.0)/1024;
  delay(1000);

  /*Lectura Sensor Magnetico Puerta Principal*/
  int puertaPrincipal = digitalRead(pinSensorPuertaPrincipal);

  /*Lectura Sensor Distancia Puerta Garaje (Mediante Funcion Auxiliar)*/
  int cm = ping(triggerPinSensorCochera,echoPinSensorCochera);
  
  /*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

  /*CODIGO DEL SERVIDOR WEB*/
  
  
  //Comprobar si hay algun cliente conectado
  WiFiClient client = server.available();

  if(!client){
    return;
  }

  //Esperar hasta que el cliente envie algun dato
  Serial.println("new client");
  while(!client.available()){
    delay(1);    
  }


  //Leer la primera linea de la peticion
  String request = client.readStringUntil('/r');
  Serial.println(request);
  client.flush();

  /*---------------------------------------------------- GET LUCES -----------------------------------------------------*/

  /*Si el led de la cocina o entrada, estan apagados, y, desde la web se pide encenderlos, se encienden.
  
  Si estan encendidos y se pide apagarlos desde la web, se apagan.*/  
  
  if(valorLedCocina == LOW && request.indexOf("/LED_COCINA=ON") != -1){
    
    digitalWrite(pinLedCocina,HIGH);
    valorLedCocina = HIGH;
       
  }else if(valorLedCocina == HIGH && request.indexOf("/LED_COCINA=OFF") != -1){
    
    digitalWrite(pinLedCocina,LOW);
    valorLedCocina = LOW;
       
  }
  
  if(valorLedEntrada == LOW && request.indexOf("/LED_ENTRADA=ON") != -1){
    
    digitalWrite(pinLedEntrada,HIGH);
    valorLedEntrada = HIGH;
    
  }else if(valorLedEntrada == HIGH && request.indexOf("/LED_ENTRADA=OFF") != -1){
    
    digitalWrite(pinLedEntrada,LOW);
    valorLedEntrada = LOW;
    
  }
  

  /*Si se pide desde la web, encender todas las luces o apagarlos, se encienden o se apagan.*/
  if(request.indexOf("/LEDS=ON") != -1){
    
    digitalWrite(pinLedCocina,HIGH);
    valorLedCocina = HIGH;

    digitalWrite(pinLedEntrada,HIGH);
    valorLedEntrada = HIGH;

    //digitalWrite(pinLedPatio,HIGH);
    //valorLedPatio = HIGH;
       
  }else if(request.indexOf("/LEDS=OFF") != -1){
    
    digitalWrite(pinLedCocina,LOW);
    valorLedCocina = LOW;

    digitalWrite(pinLedEntrada,LOW);
    valorLedEntrada = LOW;

    //digitalWrite(pinLedPatio,LOW);
    //valorLedPatio = LOW;
    
  }

  /*---------------------------------------------------- GET AC -----------------------------------------------------*/

  /*Si el led del AC esta apagado, y se pide encenderlo desde la web, o, la temperatura registrada
  por el sensor, supera los 26 grados centigrados, se enciendo el led que simula el AC.
  
  Si el led esta encendido, y, se pide apagarlo o la temperatura baja de 26 grados se apaga el led
  simulador del AC*/

  if(valorLedAC == LOW && request.indexOf("/LED_AC=ON") != -1 || celsius >= 26){
    
    digitalWrite(pinLedAC,HIGH);
    valorLedAC = HIGH;
    
  }else if(valorLedAC == HIGH && request.indexOf("/LED_AC=OFF") != -1 || celsius <= 26){
    
    digitalWrite(pinLedAC,LOW);
    valorLedAC = LOW;
    
  }


  /*---------------------------------------------------- GET PUERTAS -----------------------------------------------------*/

  /*Si existe un objeto delante de la puerta del garaje, o, desde la web de gestion se activa la puerta del garaje
  se realiza una espera de 2 segundos, y se acciona el servomotor que controla la puerta del garaje.
  
  Si se pide desde la web que se cierre la puerta, se esperan 2 segundos y se acciona el servomotor.*/
  
  if(cm <= 5 ||  request.indexOf("/GARAJE=ON") != -1){
    delay(2000);
    
    posicionServoGaraje = 90;
    servoGaraje.write(posicionServoGaraje);   //Servo en posicion 90 Grados (Abierta)       
  }else  if(request.indexOf("/GARAJE=OFF") != -1){
    delay(2000);
    
    posicionServoGaraje = 0;
    servoGaraje.write(posicionServoGaraje);    //Servo en posicion 0 Grados (Cerrada)      
  }

  /*Si el sensor magnetico esta cerrado y se pide abrir la puerta principal desde la web de gestion, se acciona el
  servomotor que controla la puerta.
  
  Si el sensor magnetico esta abierto y se pide cerrar la puerta desde la web, se acciona el servomotor para cerrar
  la puerta.*/
  
  if(puertaPrincipal == 1 && request.indexOf("/ENTRADA=ON") != -1){
    
    posicionServoEntrada = 90;
    servoEntrada.write(posicionServoEntrada);   //Servo en posicion 90 Grados (Abierta)

    puertaPrincipal = 0;    //Puerta Abierta
           
  }else  if(puertaPrincipal == 0 && request.indexOf("/ENTRADA=OFF") != -1){
    
    posicionServoEntrada = 0;
    servoEntrada.write(posicionServoEntrada);    //Servo en posicion 0 Grados (Cerrada)

    puertaPrincipal = 1;    //Puerta Cerrada
           
  }


  /*---------------------------------------------------- GET PERSIANAS -----------------------------------------------------*/

  /*Si se pide desde web subir la persiana del piso de abajo, se acciona el servomotor.
  
  Si se pide bajarla, se acciona el servomotor hasta llegar a la posicion inicial.*/
  if(request.indexOf("/PERSIANA_ABAJO=ON") != -1){

    posicionServoPersianaPisoAbajo = 90;
    servoPersianaPisoAbajo.write(posicionServoPersianaPisoAbajo);   //Servo en posicion 90 Grados (Abierta)    
       
  }else if(request.indexOf("/PERSIANA_ABAJO=OFF") != -1){

    posicionServoPersianaPisoAbajo = 0;
    servoPersianaPisoAbajo.write(posicionServoPersianaPisoAbajo);    //Servo en posicion 0 Grados (Cerrada)    
       
  }



  if(request.indexOf("/PERSIANA_ARRIBA=ON") != -1){

    posicionServoPersianaPisoArriba = 90;
    servoPersianaPisoArriba.write(posicionServoPersianaPisoArriba);   //Servo en posicion 90 Grados (Abierta)    
       
  }else  if(request.indexOf("/PERSIANA_ARRIBA=OFF") != -1){

    posicionServoPersianaPisoArriba = 0;
    servoPersianaPisoArriba.write(posicionServoPersianaPisoArriba);    //Servo en posicion 0 Grados (Cerrada)    
       
  }


  /*---------------------------------------------------- GET VENTILADOR -----------------------------------------------------*/

  /*Si se pide accionar el ventilador desde la web, se acciona el servomotor que lo controla.*/
  if(request.indexOf("/VENTILADOR=ON") != -1){

    posicionServoVentilador = 0;    //Horario(Movimiento)
    servoVentilador.write(posicionServoVentilador);   //Servo en rotacion en sentido horario (Movimiento)    
       
  }else if(request.indexOf("/VENTILADOR=OFF") != -1){

    posicionServoVentilador = 90;  //Detenido
    servoVentilador.write(posicionServoVentilador);   //Servo detenido    
       
  }


  /*CODIGO HTML DE LA PAGINA WEB DE RESPUESTA*/

  
  //Devuelve la respuesta
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Refresh: 60"); //Refrescar la pagina cada minuto
  client.println(""); //No olvidar la linea en blanco
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head>");
  client.println("<h1>Servidor Web</h1>"); 
  client.println("</head>");
  client.println("<body>");

  client.print("<div style=\"width:100%; height:100%;\">");  
  client.print("<iframe src=\"https://www.zeitverschiebung.net/clock-widget-iframe-v2? language=es&timezone=Europe%2FMadrid\" width=\"100%\" height=\"150\"></iframe>");

  client.println("<h2>Informacion de la Casa</h2>");
  client.print("<b>Temperatura: </b>");
  client.print((long)celsius);
  client.print((char)176);  //Printear simbolo grado
  client.print("C"); 
  
  client.print("<hr style=\"border:15px,width:100%;\">");
  client.print("<br>");
  
 
  client.print("<br>");
  client.print("<br>");

  /*-------------------------------------------------------------------------------*/

  client.print("<div id=\"Luces\" style=\"text-align:center; width:100%; height:200px;\"> ");
  client.print("<h2 style=\"text-decoration:underline;\">Luces</h2>");

  /*CONTROL LUCES COMEDOR/COCINA*/
  client.print("<div id=\"Luces Comedor\" style=\"width:275px; height:100px; margin-left:200px; display:inline-block;\">");
  client.print("<b>Estado Luces Comedor/Cocina:</b> ");

  if(valorLedCocina == HIGH){
    client.print("<span style=\"color:green;\"><b>ON </b></span>");  
  }else{
    client.print("<span style=\"color:red;\"><b>OFF </b></span>");  
  }

  client.print("<br><br>");
  client.print("<a href=\"/LED_COCINA=ON\"\"><button style=\"background-color:green;\"><b>On</b> </button></a>");
  client.print("<a href=\"/LED_COCINA=OFF\"\"><button style=\"background-color:red;\"><b>Off</b> </button></a>");
  client.print("</div>");
  

  /*CONTROL LUCES ENTRADA*/
  client.print("<div id=\"Luces Entrada\" style=\"width:250px; height:100px; display:inline-block;\">");
  client.print("<b>Estado Luces Entrada:</b> ");

  if(valorLedEntrada == HIGH){
    client.print("<span style=\"color:green;\"><b>ON </b></span>");  
  }else{
    client.print("<span style=\"color:red;\"><b>OFF </b></span>");  
  }

  client.print("<br><br>");
  client.print("<a href=\"/LED_ENTRADA=ON\"\"><button style=\"background-color:green;\"><b>On</b> </button></a>");
  client.print("<a href=\"/LED_ENTRADA=OFF\"\"><button style=\"background-color:red;\"><b>Off</b> </button></a>");
  client.print("</div>");


   /*CONTROL TOTAL LUCES*/
  client.print("<div id=\"Control Luces\" style=\"float:right; width:175px; height:100px; display:inline-block; margin-right:25px;\">");
  client.print("<b>Control Maestro de Luces</b>");

  client.print("<br><br>");
  client.print("<a href=\"/LEDS=ON\"\"><button style=\"background-color:green;\"><b>On</b> </button></a>");
  client.print("<a href=\"/LEDS=OFF\"\"><button style=\"background-color:red;\"><b>Off</b> </button></a>");
  client.print("</div>");
  client.print("</div>");
  

/*------------------------------------------------------------------------------------------*/

  client.print("<div id=\"Puertas\" style=\"text-align:center; width:100%; height:200px; \"> ");
  client.print("<h2 style=\"text-decoration:underline;\">Puertas</h2>");

  /*CONTROL PUERTA GARAJE*/
  client.print("<div id=\"Puerta Garaje\" style=\"width:250px; height:100px; display:inline-block;\">");
  client.print("<b>Estado Puerta Garaje:</b> ");

  if(posicionServoGaraje == 90){
    client.print("<span style=\"color:green;\"><b>Abierta </b></span>");  
  }else if(posicionServoGaraje == 0){
    client.print("<span style=\"color:red;\"><b>Cerrada </b></span>");  
  }

  client.print("<br><br>");
  client.print("<a href=\"/GARAJE=ON\"\"><button style=\"background-color:green;\"><b>Abrir </b> </button></a>");
  client.print("<a href=\"/GARAJE=OFF\"\"><button style=\"background-color:red;\"><b>Cerrar </b> </button></a>");
  client.print("</div>");
  


  /*CONTROL PUERTA ENTRADA*/
  client.print("<div id=\"Puerta Entrada\" style=\"width:250px; height:100px; display:inline-block;\">");
  client.print("<b>Estado Puerta Entrada:</b> ");

  if(puertaPrincipal == 0){
    client.print("<span style=\"color:green;\"><b>Abierta </b></span>");  
  }else if(puertaPrincipal == 1){
    client.print("<span style=\"color:red;\"><b>Cerrada </b></span>");  
  }

  client.print("<br><br>");
  client.print("<a href=\"/ENTRADA=ON\"\"><button style=\"background-color:green;\"><b>Abrir </b> </button></a>");
  client.print("<a href=\"/ENTRADA=OFF\"\"><button style=\"background-color:red;\"><b>Cerrar </b> </button></a>");
  client.print("</div>");
  client.print("</div>");
  

  /*-----------------------------------------------------------------------------------*/

  client.print("<div id=\"Persianas\" style=\"text-align:center; width:100%; height:200px;\"> ");
  client.print("<h2 style=\"text-decoration:underline;\">Persianas</h2>");

  /*CONTROL PERSIANAS COMEDOR/COCINA*/
  client.print("<div id=\"Persiana Comedor\" style=\"width:325px; height:100px; display:inline-block;\">");
  client.print("<b>Estado Persianas Comedor/Cocina:</b> ");

  if(posicionServoPersianaPisoAbajo == 90){
    client.print("<span style=\"color:green;\"><b>Abierta </b></span>");  
  }else if(posicionServoPersianaPisoAbajo == 0){
    client.print("<span style=\"color:red;\"><b>Cerrada </b></span>");  
  }

  client.print("<br><br>");
  client.print("<a href=\"/PERSIANA_ABAJO=ON\"\"><button style=\"background-color:green;\"><b>Abrir </b> </button></a>");
  client.print("<a href=\"/PERSIANA_ABAJO=OFF\"\"><button style=\"background-color:red;\"><b>Cerrar </b> </button></a>");
  client.print("</div>");

  /*CONTROL PERSIANAS HABITACION*/
  client.print("<div id=\"Persiana Habitacion\" style=\"width:300px; height:100px; display:inline-block;\">");
  client.print("<b>Estado Persianas Habitacion:</b> ");

  if(posicionServoPersianaPisoArriba == 90){
    client.print("<span style=\"color:green;\"><b>Abierta </b></span>");  
  }else if(posicionServoPersianaPisoArriba == 0){
    client.print("<span style=\"color:red;\"><b>Cerrada </b></span>");  
  }

  client.print("<br><br>");
  client.print("<a href=\"/PERSIANA_ARRIBA=ON\"\"><button style=\"background-color:green;\"><b>Abrir </b> </button></a>");
  client.print("<a href=\"/PERSIANA_ARRIBA=OFF\"\"><button style=\"background-color:red;\"><b>Cerrar </b> </button></a>");
  client.print("</div>");
  client.print("</div>");
  


  /*-----------------------------------------------------------------------------------*/

  client.print("<div id=\"Aire_Acondicionado\" style=\"text-align:center; width:100%; height:200px;\"> ");
  client.print("<h2 style=\"text-decoration:underline;\">Aire Acondicionado</h2>");

  /*CONTROL A/C COMEDOR Y HABITACION*/
  client.print("<div id=\"Aire Acondicionado\" style=\"width:400px; height:100px; display:inline-block;\">");
  client.print("<b>Estado Aire Acondicionado Comedor/Cocina:</b> ");

  if(valorLedAC == HIGH){
    client.print("<span style=\"color:green;\"><b>ON </b></span>");  
  }else{
    client.print("<span style=\"color:red;\"><b>OFF </b></span>");  
  }

  client.print("<br><br>");
  client.print("<a href=\"/LED_AC=ON\"\"><button style=\"background-color:green;\"><b>On </b> </button></a>");
  client.print("<a href=\"/LED_AC=OFF\"\"><button style=\"background-color:red;\"><b>Off </b> </button></a>");
  client.print("</div>");
  client.print("</div>");


  client.print("</div>");


  /*-----------------------------------------------------------------------------------*/

  client.print("<div id=\"Ventilador\" style=\"text-align:center; width:100%; height:200px;\"> ");
  client.print("<h2 style=\"text-decoration:underline;\">Ventilador</h2>");

  /*CONTROL VENTILADOR*/
  client.print("<div id=\"Ventilador\" style=\"width:300px; height:100px; display:inline-block;\">");
  client.print("<b>Estado Ventilador:</b> ");

  if(posicionServoVentilador == 0){
    client.print("<span style=\"color:green;\"><b>Movimiento </b></span>");  
  }else if(posicionServoVentilador == 90){
    client.print("<span style=\"color:red;\"><b>Detenido </b></span>");  
  }

  client.print("<br><br>");
  client.print("<a href=\"/VENTILADOR=ON\"\"><button style=\"background-color:green;\"><b>Abrir </b> </button></a>");
  client.print("<a href=\"/VENTILADOR=OFF\"\"><button style=\"background-color:red;\"><b>Cerrar </b> </button></a>");
  client.print("</div>");
  
  /*------------------------------------------------------------------------------------*/
  
  client.println("</body>");
  client.println("</html>");

  delay(1);
  Serial.println("Client Disconnected");
  Serial.println("");
                  
}


/*Funcion Auxiliar Sensor Distancia*/

int ping(int triggerPinSensorCochera, int echoPinSensorCochera){

  long duration,distanceCm;

  digitalWrite(triggerPinSensorCochera, LOW); //Generacion de un pulso limpio
  delayMicroseconds(4);
  digitalWrite(triggerPinSensorCochera, HIGH); //Generamos senal
  delayMicroseconds(10);
  digitalWrite(triggerPinSensorCochera,LOW);

  duration = pulseIn(echoPinSensorCochera, HIGH); //Medimos el tiempo entre pulsos

  distanceCm = duration * 10/ 292 /2; //Convertimos distancia a cm
  return distanceCm;
    
}
