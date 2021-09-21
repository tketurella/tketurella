/*
 * Desenvolvido por: Fernanda Borges Turella
 * Esse código utiliza o sensor Grove-Line Finder V1.1, LCD 20x4, botoes, motor e inversor comercial
 * Esse código tem o acionamento do motor e está sendo desenvolvido o botão return dentro do novo teste, após inserir o numero de obra
 */

//---inclui bibliotecas---
#include<LiquidCrystal.h>
#include<EEPROM.h>
#include<Wire.h>
#include <TimerOne.h> 

//--PINOS LCD--
#define RS 13
#define EN 12
#define D4 A0
#define D5 A1
#define D6 A2
#define D7 A3

//--VARIAVEIS LCD--
#define coluna 20
#define linha 4

//--------BOTOES---------
#define buttonSelect 11
#define buttonUp 10
#define buttonDown 9
#define buttonReturn 8

//--------PINOS PWM----------
#define L_EN  3
#define R_EN  4  
#define L_PWM 5 //PWM ARDUINO
#define R_PWM 6 //PWM ARDUINO

//-------------------------VARIAVEIS PWM--------------------------------------------------------
#define T 100 //tempo para delay de incremento para acionar/desacionar o motor 
#define tInc 5 //incremento de acionamento/desacionamento do motor
int rotSide = 0; //sentido de giro do motor: 0 para direita, 1 para esquerda
int dutyCycle = 255; //ciclo do PWM em valores, para ver em % fazer a divisão de 255/(valor/100)
int dutyCycleInc = 0; //incremento do ciclo
int controlDutyCycle = 0; //controle do valor do ciclo

//---------------variaveis LEITOR DE CORRENTE------------------------------------------------
#define timeInterrupt 3000 //interrupcao para atualizar o valor da corrente
#define Vmax 5 //Tensao maxima do ARDUINO UNO
#define resolutionADMax 1023 //resolucao maxima do AD ARDUINO UNO (10 bits)
#define analog A4 //PINO ANALOGICO para leitura de grandezas eletricas
float shuntReadAD = 0, Rshunt = 0.05, Vshunt = 0, Ishunt = 0; //Resistencia, Tensao e Corrente
float currentRead = 0;
float maxVResolution; //divisao entre Vmax e resolutionADMax

//----------------variaveis sensorLine-----------------------------------------------
#define sensorLine A5 //PINO SENSOR OPTICO
int countReadHigh, countReadLow; //cont do sensor
char lcdBufferFinderHigh[100], lcdBufferFinderLow[100]; //buffer do sensor para o lcd

//----------------string de menus--------------------------------------------------
String mainMenuItems[] = {"Novo Teste", "Historico", "Certificacao"};
String menuNovoTesteItems[] = {"TKE", "Outras Marcas"};
String menuHistoricoItems[] = {"Por data", "Por obra"};
String menuCertificacaoItems[] = {"Serial", "Validade"};

/*----------------variaveis de controle de menus---------------------------------
 *      main(Principal), NT(Novo Teste, H(Historico) e C(Certificacao) 
 */
int mainMenuSelect = 0, NTMenuSelect = 0, HMenuSelect = 0, CMenuSelect = 0; //controles de selecao de cada menu
int mainCursorPosition = 0, NTCursorPosition = 0, HCursorPosition = 0, CCursorPosition = 0; //posicao para desenhar cada menu
int posSO = 0; //posicao seleciona obra - para avancar no menu de Novo Teste>>TKE>>Inserir numero de obra
int vetNObra[6]; //vetor para armazenar numero de obra no menu Novo Teste>>TKE

//----------------variaveis EEPROM---------------------------------
int addwI = 0, addrI = 0; //indices de endereço para escrita e leitura
int iNObras[50]; //indice de cada número de obra
int digWriteNObra[3]; //digito de escrita de número de obra
int digReadNObra[6]; //digito de leitura de número de obra
int NObras[300];

//-----caractere especial para o cursor dos menu---------
byte menuCursor[8] = {
  B01000, //  *
  B00100, //   *
  B00010, //    *
  B00001, //     *
  B00010, //    *
  B00100, //   *
  B01000, //  *
  B00000  //
};

//seta os pinos LCD
LiquidCrystal lcd(RS,EN,D4,D5,D6,D7); 

//liga desliga led do sensor Grove-Line Finder V1.1
void lineFinderWrite(){
  digitalWrite(sensorLine,HIGH);
  delay(100);
  digitalWrite(sensorLine,LOW);
  delay(100);
}

//le o sensor Grove-Line Finder V1.1
void lineFinderRead(){
  int i = 0, j = 0;
  //se for leitura alta, conta como black
  if(HIGH == digitalRead(sensorLine)){
    countReadHigh++;
    for(i=0;i<100;i++){
      lcdBufferFinderHigh[i] = '\0';
    }
    delay(1000);
  }
  //se for leitura baixa, conta como white
  else if(LOW == digitalRead(sensorLine)){
    countReadLow++;
    for(j=0;j<100;j++){
      lcdBufferFinderLow[j] = '\0';
    }
    delay(1000);
  }
}

//imprime o valor de leitura e leitura alta ou baixa
void printLineFinder(int line, int col){
  //se for leitura alta, black
  if(HIGH == digitalRead(sensorLine)){
    lcd.setCursor(line,col);
    lcd.print("                  ");
    lcd.setCursor(line,col);
    sprintf(lcdBufferFinderHigh, "black %i", countReadHigh);
    lcd.print(lcdBufferFinderHigh);
  }
  //se for leitura baixa, white
  else if(LOW == digitalRead(sensorLine)){
    lcd.setCursor(line,col);
    lcd.print("                  ");
    lcd.setCursor(line,col);
    sprintf(lcdBufferFinderLow, "white %i", countReadLow);
    lcd.print(lcdBufferFinderLow);
  }
}

//Primeira tela do sistema
void startLCD(){
  lcd.clear();
  lcd.setCursor(8,1);
  lcd.print("TKE");
  lcd.setCursor(7,2);
  lcd.print("OGTv2");
  delay(1000);
}

//---------------------------------------------------ESCOLHA BOTAO-------------------------------------------------------------------------------------
int menuSelectButton(){
  int stateMainMenu = 0;

  //nenhum botao selecionado
  if(digitalRead(buttonDown) == HIGH && digitalRead(buttonUp) == HIGH && digitalRead(buttonReturn) == HIGH && digitalRead(buttonSelect) == HIGH){
    stateMainMenu = 0;
  }
  //botao down selecionado
  else if(digitalRead(buttonDown) == LOW && digitalRead(buttonUp) == HIGH && digitalRead(buttonReturn) == HIGH && digitalRead(buttonSelect) == HIGH){
    stateMainMenu = 1;
  }
  //botao up selecionado
  else if(digitalRead(buttonDown) == HIGH && digitalRead(buttonUp) == LOW && digitalRead(buttonReturn) == HIGH && digitalRead(buttonSelect) == HIGH){
    stateMainMenu = 2;
  }
  //botao voltar selecionado
  else if(digitalRead(buttonDown) == HIGH && digitalRead(buttonUp) == HIGH && digitalRead(buttonReturn) == LOW && digitalRead(buttonSelect) == HIGH){
    stateMainMenu = 3;
  }
  //botao selecionar selecionado
  else if(digitalRead(buttonDown) == HIGH && digitalRead(buttonUp) == HIGH && digitalRead(buttonReturn) == HIGH && digitalRead(buttonSelect) == LOW){
    stateMainMenu = 4;
  }
  return stateMainMenu;
}

//-------------------------PWM--------------------------------------
//aciona motor no sentido da esquerda
void motorStartLeft(int controlInc){
  int control;
  
  for(control = controlInc; control < dutyCycleInc+1; control+=tInc){
    analogWrite(L_PWM, control);
    lcd.setCursor(12,1);
    lcd.print ("    ");
    lcd.setCursor(12,1);
    lcd.print (control);    
    delay(T);
  }
}

//desaciona motor no sentido da esquerda
void motorStopLeft(int controlInc){
  int control;
  
  for(control = dutyCycleInc; control >= controlInc; control-=tInc){
    analogWrite(L_PWM, control);   
    lcd.setCursor(12,1);
    lcd.print ("    ");
    lcd.setCursor(12,1);
    lcd.print (control);    
    delay(T);
  }
}

//aciona motor no sentido da direita
void motorStartRight(int controlInc){
  int control;
  
  for(control = controlInc; control < dutyCycleInc+1; control+=tInc){
    analogWrite(R_PWM, control);
    lcd.setCursor(12,1);
    lcd.print ("    ");
    lcd.setCursor(12,1);
    lcd.print (control);    
    delay(T);
  }
}

//desaciona motor no sentido da direita
void motorStopRight(int controlInc){
  int control;
  
  for(control = dutyCycleInc; control >= controlInc; control-=tInc){
    analogWrite(R_PWM, control);
    lcd.setCursor(12,1);
    lcd.print ("    ");
    lcd.setCursor(12,1);
    lcd.print (control);    
    delay(T);
  } 
}

//escolher direcao e aciona/desaciona o motor
void operateMotor(){
  int button = 0, activeButton = 0, control = 0; 

  lcd.setCursor(0,1);
  lcd.print("DUTY CYCLE:");

  digitalWrite(R_EN, HIGH);
  digitalWrite(L_EN, HIGH);

  lcd.setCursor(0,3);
  lcd.print("CORRENTE: ");

  while(activeButton == 0){
    lcd.setCursor(10,3);
    lcd.print(currentRead);
    button = menuSelectButton();
    delay(100);
    switch(button){
      case 0:
        lcd.setCursor(0,2);
        if (rotSide == 1) lcd.print ("SENTIDO: Esquerda");
        if (rotSide == 0) lcd.print ("SENTIDO: Direita ");        
      break;
      //botao baixo - Down -  desaciona motor
      case 1:
        lcd.setCursor(0,2);
        if (rotSide == 1) lcd.print ("SENTIDO: Esquerda");
        if (rotSide == 0) lcd.print ("SENTIDO: Direita ");  
        dutyCycleInc-=tInc;
        controlDutyCycle = dutyCycleInc;
        if(dutyCycleInc < 0){
          dutyCycleInc = 0;
          controlDutyCycle = 0;
        }
        //DIREITA
        if (rotSide == 0){
          motorStopRight(controlDutyCycle);
        }
        //ESQUERDA
        else if (rotSide == 1){
          motorStopLeft(controlDutyCycle);
        }
      break;
     //botao cima - Up - aciona motor de acordo com a direcao escolhida
      case 2:
        lcd.setCursor(0,2);
        if (rotSide == 1) lcd.print ("SENTIDO: Esquerda");
        if (rotSide == 0) lcd.print ("SENTIDO: Direita ");       
        dutyCycleInc += tInc;
        controlDutyCycle = dutyCycleInc;
        if(dutyCycleInc >= dutyCycle+1) dutyCycleInc = dutyCycle;
        //aciona o motor para o sentido escolhido, travando o oposto
        if(rotSide == 0){//direita          
          motorStartRight(controlDutyCycle);
        }
        else if(rotSide == 1){//esquerda
          motorStartLeft(controlDutyCycle);
        }
      break;      
      //botao Esquerda - Return 
      case 3:  
        if (rotSide == 1){
          motorStopLeft(0);
          controlDutyCycle = 0;
          dutyCycleInc = 0;           
        }
        if (rotSide == 0){ 
          motorStopRight(0);    
          controlDutyCycle = 0;
          dutyCycleInc = 0;            
        }    
        menuObraDraw(vetNObra);
        activeButton = 1;
      break;      
      //botao Select - para giro: 1 vez aciona direita, 2 vezes aciona esquerda 
      case 4:      
        lcd.setCursor(0,2);
        //mostrar o sentido e parar o motor caso esteja acionado para o sentido oposto do escolhido
        if (rotSide == 1){
          lcd.print ("SENTIDO: Esquerda");
          motorStopLeft(0);
          controlDutyCycle = 0;
          dutyCycleInc = 0;           
        }
        if (rotSide == 0){
          lcd.print ("SENTIDO: Direita ");  
          motorStopRight(0);    
          controlDutyCycle = 0;
          dutyCycleInc = 0;            
        } 
        //seleciona sentido de giro para direita
        rotSide = 0; 
        //para selecionar sentido esquerdo
        button = menuSelectButton();
        delay(50);
        switch(button){
          case 0:
          break;
          case 1:
          break;
          case 2:
          break;
          case 3:
          break;
          case 4:         
            lcd.setCursor(0,2);
            //mostrar o sentido e parar o motor caso esteja acionado para o sentido oposto do escolhido
            if (rotSide == 1){
              lcd.print ("SENTIDO: Esquerda");
              motorStopLeft(0);
              controlDutyCycle = 0;
              dutyCycleInc = 0;           
            }
            if (rotSide == 0){
              lcd.print ("SENTIDO: Direita ");  
              motorStopRight(0);    
              controlDutyCycle = 0;
              dutyCycleInc = 0;            
            }  
            //seleciona sentido de giro para esquerda
            rotSide = 1;   
          break;
        }
      break; 
    }
  }
}


//---------------LEITOR DE CORRENTE----------------------
float functionCurrentRead(){
  shuntReadAD = analogRead(analog);
  //maxVResolution = Vmax/resolutionADMax; 
  maxVResolution = 0.00488; //maxVResolution = 5/1023
  Ishunt = shuntReadAD*maxVResolution*(1/Rshunt);   
  return Ishunt;
}

//**-------------menu Inicio-----------------------------
//gera o menu principal e indica opcao a ser selecionada
void mainMenuDraw() {
  //mostra o primeiro menu no LCD
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("Inicio");
  lcd.setCursor(1, 1);
  lcd.print(mainMenuItems[0]); //Novo Teste
  lcd.setCursor(1, 2);
  lcd.print(mainMenuItems[1]); //Historico
  lcd.setCursor(1, 3);
  lcd.print(mainMenuItems[2]); //Certificacao
}

//desenha cursor menu principal
void drawMainCursor() {
  clearCursor();
 
  //Novo Teste
  if (mainMenuSelect == 0) {
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
      mainCursorPosition = 0;
  }
  //Historico
  else if (mainMenuSelect == 1) {
      lcd.setCursor(0, 2);
      lcd.write(byte(0));
      mainCursorPosition = 1;
  }
  //Certificacao
  else if (mainMenuSelect == 2) { 
      lcd.setCursor(0, 3);
      lcd.write(byte(0));
      mainCursorPosition = 2;
  }
}

//limpa os cursores do lcd
void clearCursor(){
  int x;
  for(x = 0; x < 4; x++){
    lcd.setCursor(0, x);
    lcd.print(" ");
  }
}

//funcionamento menu principal
void operateMainMenu() {
  int activeButton = 0;
  
  while(activeButton == 0) {
    int button;
    button = menuSelectButton();
    delay(150);
    switch (button) {
      //nenhum botao for selecionado
      case 0:
      break;
      //botao down acionado
      case 1:
        mainMenuDraw();
        switch(mainMenuSelect){
          case 0:
            drawMainCursor();
            mainMenuSelect = 1;
          break;
          case 1:
            drawMainCursor();  
            mainMenuSelect = 2;
          break;
          case 2:
            drawMainCursor();
            mainMenuSelect = 0;
          break;
        }
        activeButton = 1;
      break;
      //botao up acionado
      case 2:
        mainMenuDraw();
        switch(mainMenuSelect){
          case 0:
            drawMainCursor();
            mainMenuSelect = 2;
          break;
          case 1:
            drawMainCursor();
            mainMenuSelect = 0; 
          break;
          case 2:
            drawMainCursor();
            mainMenuSelect = 1;  
          break;
        }
        activeButton = 1;
      break;
      //botao voltar acionado
      case 3:
        mainMenuDraw();
        drawMainCursor();
        activeButton = 1;
      break;
      //botao selecionar acionado
      case 4:
        switch(mainCursorPosition){
          case 0:
            operateNovoTesteMenu();
          break;
          case 1:
            operateHistoricoMenu();
          break;
          case 2:
            operateCertificacaoMenu();          
          break;
        }
        activeButton = 1;
      break;
    }
  }
}

//**-------------menu Novo Teste---------------------------
//gera o menu Novo Teste e indica opcao a ser selecionada
void novoTesteMenuDraw(){
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Inicio>Novo Teste");
  lcd.setCursor(1, 1);
  lcd.print(menuNovoTesteItems[0]); //TKE
  lcd.setCursor(1, 2);
  lcd.print(menuNovoTesteItems[1]); //Outras Marcas
}

//desenha cursor menu Novo Teste
void drawNovoTesteCursor() {
  clearCursor();
 
  //TKE
  if (NTMenuSelect == 0) {
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
      NTCursorPosition = 0;
  }
  //Outras Marcas
  else if (NTMenuSelect == 1) {
      lcd.setCursor(0, 2);
      lcd.write(byte(0));
      NTCursorPosition = 1;
  }
}

//seleciona um novo teste TKE ou outras marcas
void operateNovoTesteMenu() {
  int activeButton = 0;
  
  novoTesteMenuDraw();
    
  while (activeButton == 0){
    int button;
    button = menuSelectButton();
    delay(150);
    switch (button){
      case 0:
      break;
      //botao down acionado
      case 1:
        delay(150);
        switch(NTMenuSelect){
          case 0:
            drawNovoTesteCursor();
            NTMenuSelect = 1;
          break;
          case 1:
            drawNovoTesteCursor();
            NTMenuSelect = 0;
          break;
        }
      break;
      //botao up acionado
      case 2:
        delay(150);
        switch(NTMenuSelect){
          case 0:
            drawNovoTesteCursor();
            NTMenuSelect = 1;
          break;
          case 1:
            drawNovoTesteCursor();
            NTMenuSelect = 0;
          break;
        }
      break;
      //botao return acionado
      case 3:  
        novoTesteMenuDraw();
        drawNovoTesteCursor();
        activeButton = 1;
      break;
      //botao select acionado
      case 4:
        switch(NTCursorPosition){
          case 0:
            operateSelectObraTKEMenu();
          break;
          case 1:
            operateOutrasMarcasMenu();     
          break;
        }
        activeButton = 1;
      break;
    }
  }
}

//mostra a primeira tela do menu para inserir numero de obra
void menuSelectObraDraw(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Novo Teste>TKE>Obra");
  lcd.setCursor(0, 1);
  lcd.print("Insira N obra");
  lcd.setCursor(0, 2);
  lcd.print("------");
  lcd.setCursor(1, 3);
  lcd.print("Avancar");
}

//mostra o digito escolhido para o numero de obra para cada posicao
void nDraw(int pos, int n){
  lcd.setCursor(pos,2);
  lcd.print(n);
}

//mostra numero de obra inserido
void menuObraDraw(int nObra[6]){
  int i;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Novo Teste>TKE>Obra");
  lcd.setCursor(0, 1);
  lcd.print("Insira N obra");
  for(i = 0; i < 6; i++){
    lcd.setCursor(i, 2);
    lcd.print(nObra[i]);
  }
  lcd.setCursor(1, 3);
  lcd.print("Avancar");
}

//desenha cursor da opcao avancar - menu selecão número de obra
void drawNTSelectObraTKECursor() {
 
  //opção Avancar
  if (posSO > 5) {
      lcd.setCursor(0, 3);
      lcd.write(byte(0));
  }
}

/*
 * rotina chamada na função operateNovoTesteMenu()
 * insere número de obra e inicia teste 
 * teste inicial: acionar polia com motor
 */
void operateSelectObraTKEMenu(){
  int n0 = 0, n1 = 0, n2 = 0, n3 = 0, n4 = 0, n5 = 0;
  int pos = 0, button = 0, activeButton = 0;

  menuSelectObraDraw();
    
  while(activeButton == 0){
    button = menuSelectButton();
    delay(150);
    switch(button){
      //nenhum botao acionado
      case 0:
      break;
      //botao down acionado
      case 1:
      break;
      //botao up acionado
      case 2:
      break;
      //botao return acionado
      case 3:
        novoTesteMenuDraw();
        drawNovoTesteCursor();
        pos = 7;//21/09
        if(addwI > 0) addwI--;//21/09 
        if(addrI > 0) addrI--;//21/09
        activeButton = 1;
        
      break;
      //botao select acionado
      case 4:      
        nDraw(pos,0);
        if(pos < 7){
          while(1){
            switch(pos){
              delay(100);           
              case 0:
                button = menuSelectButton();
                delay(150);
                switch(button){
                  case 0:
                  break;
                  //se botao down acionado
                  case 1:
                    if(n0 > 0) n0--;
                    else if(n0 < 0) n0 = 0;
                    nDraw(pos,n0);
                  break;
                  //se botao up acionado
                  case 2:
                    if(n0 < 9) n0++;
                    else if(n0 > 9) n0 = 0;
                    nDraw(pos,n0);
                  break;
                  //botao return acionado
                  case 3:
                    n0 = 0;
                    lcd.setCursor(pos,2);
                    lcd.print("-");
                    pos = 7;   
                    if(addwI > 0) addwI--;//21/09
                    if(addrI > 0) addrI--;//21/09
                    activeButton = 1;
                  break;
                  //botao select acionado
                  case 4:
                    if(n0 > 0) n0 = n0 * 10; //15/09
                    pos++;
                    nDraw(pos,0);
                    vetNObra[0] = n0;
                  break;
                }
              break;
              case 1:
                button = menuSelectButton();
                delay(150);
                switch(button){
                  case 0:
                  break;
                  //se botao down acionado
                  case 1:
                    if(n1 > 0) n1--;
                    else if(n1 < 0) n1 = 0;
                    nDraw(pos,n1);
                  break;
                  //se botao up acionado
                  case 2:
                    if(n1 < 9) n1++;
                    else if(n1 > 9) n1 = 0;
                    nDraw(pos,n1);
                  break;
                  //botao return acionado
                  case 3:
                    n1 = 0;
                    lcd.setCursor(pos,2);
                    lcd.print("-");
                    pos--;
                    if(addwI > 0) addwI--;//21/09 
                    if(addrI > 0) addrI--;//21/09                   
                  break;
                  //botao select acionado
                  case 4:
                    pos++;
                    nDraw(pos,0);
                    digWriteNObra[0] = (byte*)(n0 + n1);        
                    EEPROM.write(addwI,digWriteNObra[0]);
                    Serial.print("addwI - ");    
                    Serial.print(addwI);
                    Serial.print(" - digWriteNObra = ");
                    Serial.println(digWriteNObra[addwI]);
                    NObras[addrI] = (int*)(EEPROM.read(addwI)/10);
                    Serial.print("addwI - ");    
                    Serial.println(addwI);
                    Serial.print("addrI - ");    
                    Serial.println(addrI);  
                    Serial.print("NObras[addrI] - ");                
                    Serial.println(NObras[addrI]);                  
                    addrI++;                
                    NObras[addrI] = (int*)(EEPROM.read(addwI)%10);
                    Serial.print("addrI - ");     
                    Serial.println(addrI);            
                    Serial.print("NObras[addrI] - ");        
                    Serial.println(NObras[addrI]); 
                    addrI++;
                    addwI++;                
                    vetNObra[1] = n1;
                    Serial.print("addwI - ");    
                    Serial.println(addwI);
                    Serial.print("addrI - ");   
                    Serial.println(addrI); 
                    Serial.println(" ");     
                  break;
                }
              break;
              case 2:
                button = menuSelectButton();
                delay(150);
                switch(button){
                  case 0:
                  break;
                  //se botao down acionado
                  case 1:
                    if(n2 > 0) n2--;
                    else if(n2 < 0) n2 = 0;
                    nDraw(pos,n2);
                  break;
                  //se botao up acionado
                  case 2:
                    if(n2 < 9) n2++;
                    else if(n2 > 9) n2 = 0;
                    nDraw(pos,n2);
                  break;
                  //botao return acionado
                  case 3:
                    n2 = 0;
                    lcd.setCursor(pos,2);
                    lcd.print("-");
                    pos--;
                    if(addwI > 0) addwI--;
                    if(addrI > 0) addrI--;//21/09
                  break;
                  //botao select acionado
                  case 4:
                    if(n2 > 0) n2 = n2 * 10; //15/09
                    pos++;
                    nDraw(pos,0);
                    vetNObra[2] = n2;
                  break;
                }
              break;
              case 3:
                button = menuSelectButton();
                delay(150);
                switch(button){
                  //nenhum botao acionado
                  case 0:
                  break;
                  //se botao down acionado
                  case 1:
                    if(n3 > 0) n3--;
                    else if(n3 < 0) n3 = 0;
                    nDraw(pos,n3);
                  break;
                  //se botao up acionado
                  case 2:
                    if(n3 < 9) n3++;
                    else if(n3 > 9) n3 = 0;
                    nDraw(pos,n3);
                  break;
                  //botao return acionado
                  case 3:
                    n3 = 0;
                    lcd.setCursor(pos,2);
                    lcd.print("-");
                    pos--;
                    if(addwI > 0) addwI--;
                    if(addrI > 0) addrI--;//21/09
                  break;
                  //botao select acionado
                  case 4:           
                    pos++;
                    nDraw(pos,0);
                    digWriteNObra[1] = (byte*)(n2 + n3);                    
                    EEPROM.write(addwI,digWriteNObra[1]); 
                    Serial.print("addwI - ");    
                    Serial.print(addwI);
                    Serial.print(" - digWriteNObra = ");
                    Serial.println(digWriteNObra[addwI]);          
                    NObras[addrI] = (int*)(EEPROM.read(addwI)/10);                      
                    Serial.print("addwI - ");    
                    Serial.println(addwI);
                    Serial.print("addrI - ");    
                    Serial.println(addrI);  
                    Serial.print("NObras[addrI] - ");                
                    Serial.println(NObras[addrI]);                  
                    addrI++;                
                    NObras[addrI] = (int*)(EEPROM.read(addwI)%10);
                    Serial.print("addrI - ");     
                    Serial.println(addrI);            
                    Serial.print("NObras[addrI] - ");        
                    Serial.println(NObras[addrI]); 
                    addrI++;
                    addwI++;                
                    vetNObra[3] = n3;
                    Serial.print("addwI - ");    
                    Serial.println(addwI);
                    Serial.print("addrI - ");   
                    Serial.println(addrI); 
                    Serial.println(" ");       
                  break;
                }
              break;
              case 4:
                button = menuSelectButton();
                delay(150);
                switch(button){
                  case 0:
                  break;
                  //se botao down acionado
                  case 1:
                    if(n4 > 0) n4--;
                    else if(n4 < 0) n4 = 0;
                    nDraw(pos,n4);
                  break;
                  //se botao up acionado
                  case 2:
                    if(n4 < 9) n4++;
                    else if(n4 > 9) n4 = 0;
                    nDraw(pos,n4);
                  break;
                  //botao return acionado
                  case 3:
                    n4 = 0;
                    lcd.setCursor(pos,2);
                    lcd.print("-");
                    pos--;
                    if(addwI > 0) addwI--;
                  break;
                  //botao select acionado
                  case 4:
                    if(n4 > 0) n4 = n4 * 10; //15/09
                    pos++;
                    nDraw(pos,0);
                    vetNObra[4] = n4;
                  break;
                }
              break;  
              case 5:
                button = menuSelectButton();
                delay(150);
                switch(button){
                  case 0:
                  break;
                  //se botao down acionado
                  case 1:
                    if(n5 > 0) n5--;
                    else if(n5 < 0) n5 = 0;
                    nDraw(pos,n5);
                  break;
                  //se botao up acionado
                  case 2:
                    if(n5 < 9) n5++;
                    else if(n5 > 9) n5 = 0;
                    nDraw(pos,n5);
                  break;
                  //botao return acionado
                  case 3:
                    n5 = 0;
                    lcd.setCursor(pos,2);
                    lcd.print("-");
                    pos--;
                    if(addwI > 0) addwI--;
                    if(addrI > 0) addrI--;//21/09
                  break;
                  //botao select acionado
                  case 4:         
                    pos++;
                    posSO = pos; // para o cursor ir para a opção avançar
                    digWriteNObra[2] = (byte*)(n4 + n5);                   
                    EEPROM.write(addwI,digWriteNObra[2]);
                    Serial.print("addwI - ");    
                    Serial.print(addwI);
                    Serial.print(" - digWriteNObra = ");
                    Serial.println(digWriteNObra[addwI]);               
                    NObras[addrI] = (int*)(EEPROM.read(addwI)/10);
                    Serial.print("addwI - ");    
                    Serial.println(addwI);
                    Serial.print("addrI - ");    
                    Serial.println(addrI);  
                    Serial.print("NObras[addrI] - ");                
                    Serial.println(NObras[addrI]);                  
                    addrI++;                
                    NObras[addrI] = (int*)(EEPROM.read(addwI)%10);
                    Serial.print("addrI - ");     
                    Serial.println(addrI);            
                    Serial.print("NObras[addrI] - ");        
                    Serial.println(NObras[addrI]); 
                    addrI++;
                    addwI++;                
                    Serial.print("addwI - ");    
                    Serial.println(addwI);
                    Serial.print("addrI - ");   
                    Serial.println(addrI);    
                    Serial.println(" ");    
                    Serial.println("--------------AGAIN-------------------");
                    Serial.println(" ");    
                    vetNObra[5] = n5;                 
                    drawNTSelectObraTKECursor();
                  break;
                }
              break; 
              //opção avançar é uma nova posição 
              case 6:
                button = menuSelectButton();
                delay(150);
                switch(button){
                  case 0:
                  break;
                  case 1:
                  break;
                  case 2:
                  break;
                  //Return acionado
                  case 3:
                    pos--;
                    if(addwI > 0) addwI--;
                    if(addrI > 0) addrI--;//21/09
                    novoTesteMenuDraw();
                    drawNovoTesteCursor();
                    activeButton = 1;          
                  break;
                  //Select acionado
                  case 4:
                    pos++;
                    lcd.setCursor(0,0);
                    lcd.print("Novo Teste>TKE>Motor");
                    lcd.setCursor(0,1);
                    lcd.print("                    ");
                    lcd.setCursor(0,2);
                    lcd.print("                    ");
                    lcd.setCursor(0,3);
                    lcd.print("                    ");
                    operateMotor();
                    activeButton = 1; 
                  break;
                }
              break;       
            }
            if(activeButton == 1) break;
          }                  
        }
        if(pos == 7) {
          pos = 0;
          posSO = 0;
        }
        if(pos == 8) button = 3;
      break;
    }
  }
}

/* 
 * função para fazer os testes com o sensor óptico 
 * para usar: chamar a rotina na função operateSelectObraTKEMenu()
 * em button 4 >> pos 5 >> button 4 antes do activeButton
 */
void operateInicioTeste(){
  lcd.clear();
  while(1){
    if(menuSelectButton() == 3)
      break;
    lcd.setCursor(4,0);
    lcd.print("Novo Teste");
    lcd.setCursor(7,1);
    lcd.print("TKE");
    lineFinderRead();
    printLineFinder(4, 2);      
  }
  countReadHigh = 0;
  countReadLow = 0;
}

//menu de novo teste para reguladores de outras marcas que não seja TKE
void operateOutrasMarcasMenu(){
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("Novo Teste");
  delay(50);
  lcd.setCursor(1,1);
  lcd.print("Outras Marcas");
}

//**-------------menu Historico-------------------------
//gera o menu Historico e indica opcao a ser selecionada
void historicoMenuDraw() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Inicio>Historico");
  lcd.setCursor(1, 1);
  lcd.print(menuHistoricoItems[0]); //Por data
  lcd.setCursor(1, 2);
  lcd.print(menuHistoricoItems[1]); //Por obra
}

//desenha cursor menu Historico
void drawHistoricoMenuCursor() {
  clearCursor();
 
  //TKE
  if (HMenuSelect == 0) {
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
      HCursorPosition = 0;
  }
  //Outras Marcas
  else if (HMenuSelect == 1) {
      lcd.setCursor(0, 2);
      lcd.write(byte(0));
      HCursorPosition = 1;
  }
}

//menu historico: mostra os dados inseridos em testes realizados, por numero de obra ou por data
void operateHistoricoMenu() {
  int activeButton = 0;
  
  historicoMenuDraw();
  
  while (activeButton == 0) {
    int button;
    button = menuSelectButton();
    delay(150);
    switch (button) {
      case 0:
      break;
      //botao down acionado
      case 1:
        //historicoMenuDraw();
        delay(100);
        switch(HMenuSelect){
          case 0:
            drawHistoricoMenuCursor();
            HMenuSelect = 1;
          break;
          case 1:
            drawHistoricoMenuCursor();
            HMenuSelect = 0;
          break;
        }
      break;
      //botao up acionado
      case 2:
        //historicoMenuDraw();
        delay(100);
        switch(HMenuSelect){
          case 0:
            drawHistoricoMenuCursor();
            HMenuSelect = 1;
          break;
          case 1:
            drawHistoricoMenuCursor();
            HMenuSelect = 0;
          break;
        }
      break;
      //botao voltar acionado, retorna para menu inicio
      case 3:  
        delay(100);
        historicoMenuDraw();
        drawHistoricoMenuCursor();
        activeButton = 1;
      break;
      //botao selecionar acionado
      case 4:
        switch(HCursorPosition){
          case 0:
            operatePorDataMenu();
          break;
          case 1:
            operatePorObraMenu();
          break;
        }
        activeButton = 1;
      break;
    }
  }
}

//mostra no menu historico os dados de acordo com a data selecionada
void operatePorDataMenu(){  
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("Historico");
  delay(100);
  lcd.setCursor(1,1);
  lcd.print("Por Data");
  lcd.setCursor(1,2);
  lcd.print("xx/xx/xx");
}

//mostra no menu historico os dados de acordo com o numero de obra selecionada
void operatePorObraMenu(){
  int count, count0, count1, button = 0, activeButton = 0;
     
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("Historico");
  delay(100);
  lcd.setCursor(1,1);
  lcd.print("Por Obra");

  /*while (activeButton == 0) {
    button = menuSelectButton();
    delay(100);
    switch (button) {
       //nenhum botao selecionado
      case 0:
        //nao faz nada
      break;
      //botao down acionado
      case 1:
        //insere cursor para obra anterior, se estiver na primeira, irá ficar parado
        
      break;
      //botao up acionado
      case 2:
        //insere cursor na proxima obra
      break;
      //botao return acionado
      case 3:
        //retorna para menu Historico para escolher por data ou por obra
      break;
      //botao select acionado
      case 4:
        //seleciona a obra marcada pelo cursor para ver as informacoes de historico desta obra
      break;                        
    }
  }*/


for(count = addrI-6; count < addrI; count++){
  for(count0 = 1;count0 < 7; count0++){
    lcd.setCursor(count0,2);
    lcd.print(NObras[count]);
    Serial.println(NObras[count]);
  
    lcd.setCursor(count0,3);
    lcd.print(NObras[count+6]);
    Serial.println(NObras[count+6]);
  }
}
 

}

//**-------------menu Certificacao-------------------------
//gera o menu Certificacao e indica opcao a ser selecionada
void certificacaoMenuDraw() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Inicio>Certificacao");
  lcd.setCursor(1, 1);
  lcd.print(menuCertificacaoItems[0]); //Serial
  lcd.setCursor(1, 2);
  lcd.print(menuCertificacaoItems[1]); //Validade
}

//desenha cursor menu Certificacao
void drawCertificacaoCursor() {
  clearCursor();
 
  //Serial
  if (CMenuSelect == 0) {
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
      CCursorPosition = 0;
  }
  //Validade
  else if (CMenuSelect == 1) {
      lcd.setCursor(0, 2);
      lcd.write(byte(0));
      CCursorPosition = 1;
  }
}

//menu certificacao: salvar os dados adms do OGT: serial e validade de certificacao
void operateCertificacaoMenu() {
  int activeButton = 0;
  
  certificacaoMenuDraw();
  
  while (activeButton == 0) {
    int button;
    button = menuSelectButton();
    delay(150);
    switch (button) {
      case 0:
      break;
      //botao down acionado
      case 1:
        certificacaoMenuDraw();
        delay(100);
        switch(CMenuSelect){
          case 0:
            drawCertificacaoCursor();
            CMenuSelect = 1;
          break;
          case 1:
            drawCertificacaoCursor();
            CMenuSelect = 0;
          break;
        }
      break;
      //botao up acionado
      case 2:
        certificacaoMenuDraw();
        delay(100);
        switch(CMenuSelect){
          case 0:
            drawCertificacaoCursor();
            CMenuSelect = 1;
          break;
          case 1:
            drawCertificacaoCursor();
            CMenuSelect = 0;
          break;
        }
      break;
      //botao voltar acionado, retorna para menu inicio
      case 3:  
        activeButton = 1;
        certificacaoMenuDraw();
        drawCertificacaoCursor();
      break;
      //botao selecionar acionado
      case 4:
        switch(CCursorPosition){
          case 0:
            operateSerialMenu();
          break;
          case 1:
            operateValidadeMenu();
          break;
        }
    }
  }
}

//menu para salvar serial
void operateSerialMenu(){
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("Certificacao");
  delay(50);
  lcd.setCursor(1,1);
  lcd.print("Serial");
}

//menu para salvar/ver a validade de certificacao
void operateValidadeMenu(){
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("Certificacao");
  delay(100);
  lcd.setCursor(1,1);
  lcd.print("Validade");
}

/*
 * Memória EEPROM
 * fazer escrita e leitura de todos os dados
 */
//leitura numero obra na EEPROM
void readNObraEEPROM(int indStart, int indEnd){
  int addr;
  
  for(addr = indStart; addr < indEnd; addr++){
  }
}

//leitura data da obra na EEPROM
void readDateObraEEPROM(int indStart, int indEnd){
  int addr;
  
  for(addr = indStart; addr < indEnd; addr++){
  }
}

//funcao chamada pela interrupcao a cada timeInterrupt microsegundos
void callback(){
  currentRead = functionCurrentRead();
}

//*---------------SETUP PRINCIPAL------------------*//
void setup() {
  //criacao dos bytes para os caracteres
  lcd.createChar(0, menuCursor);

  //modo dos pinos
  pinMode(sensorLine,INPUT);
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonReturn, INPUT_PULLUP);
  pinMode(buttonSelect, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);
  pinMode(R_PWM, OUTPUT);
  pinMode(L_PWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);
 
  //inicializacao
  Serial.begin(9600);
  lcd.begin(coluna, linha);
  lcd.clear();
  startLCD();
  mainMenuDraw();
  drawMainCursor(); 
  //Timer1.initialize(timeInterrupt); // Inicializa o Timer1 e configura para um período de 30 microsegundos
  //Timer1.attachInterrupt(callback); // Configura a função callback() como a função para ser chamada a cada interrupção do Timer1  
}

//*---------------LOOP PRINCIPAL------------------*//
void loop() {
  operateMainMenu();
 }
