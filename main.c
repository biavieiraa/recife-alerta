// --- CONFIGURAÇÕES DO FAROL RGB ---
const int FAROL_R = 8;
const int FAROL_G = 9;
const int FAROL_B = 10;

// --- CONFIGURAÇÕES DE HARDWARE ---
const int NUM_JOGADORES = 6; 

struct Controle {
  int b1; int id1;
  int b2; int id2;
  int b3; int id3;
  int b4; int id4;
  int b5; int id5;
  int pinoVibracao; 
};

// Mapeamento dos fios e pinos dos motores de vibração 
Controle controles[NUM_JOGADORES] = {
  { 22, 1, 23, 2, 24, 3, 25, 4, 26, 5,   6 }, // J1
  { 27, 0, 28, 2, 29, 3, 30, 4, 31, 5,   5 }, // J2
  { 32, 0, 33, 1, 34, 3, 35, 4, 36, 5,   4 }, // J3
  { 37, 0, 38, 1, 39, 2, 40, 4, 41, 5,   3 }, // J4
  { 42, 0, 43, 1, 44, 2, 45, 3, 46, 5,   2 }, // J5
  { 47, 0, 48, 1, 49, 2, 50, 3, 51, 4,   7 }  // J6 
};

// --- VARIÁVEIS DO JOGO ---
enum Papel { INOCENTE, DETETIVE, ASSASSINO };
enum FaseJogo { SORTEIO, NOITE, RESOLUCAO_NOITE, AMANHECER, VITORIA_ASSASSINO, VITORIA_INOCENTES };

Papel papeis[NUM_JOGADORES];
bool vivo[NUM_JOGADORES];
FaseJogo faseAtual = SORTEIO;

int idAssassino;
int idDetetive;
int mortoAssassino = -1;
int mortoVotacao = -1;

// Variáveis para a votação simultânea
int alvoEscolhido[NUM_JOGADORES];
bool jaVotou[NUM_JOGADORES];

void mudarFarol(int r, int g, int b);
void oscilarFarolNoite(); // Nova função para o efeito pisca/oscila da noite
void vibrarFeedback(int idJogador, int vezes, int tempo); 
int checarCliqueControle(int idJogador);
void checarVitoria();
void iniciarPartida();
void acaoNoite();
void resolucaoNoite();
void amanhecer();

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(A0));

  pinMode(FAROL_R, OUTPUT);
  pinMode(FAROL_G, OUTPUT);
  pinMode(FAROL_B, OUTPUT);

  for (int i = 0; i < NUM_JOGADORES; i++) {
    pinMode(controles[i].pinoVibracao, OUTPUT);
    pinMode(controles[i].b1, INPUT_PULLUP);
    pinMode(controles[i].b2, INPUT_PULLUP);
    pinMode(controles[i].b3, INPUT_PULLUP);
    pinMode(controles[i].b4, INPUT_PULLUP);
    pinMode(controles[i].b5, INPUT_PULLUP);
    vivo[i] = true;
  }
  
  Serial.println("=== SISTEMA INICIADO ===");
}

void loop() {
  if (faseAtual == SORTEIO) {
    mudarFarol(HIGH, HIGH, LOW); // AMARELO (Vermelho + Verde) -> Distribuição de funções
    iniciarPartida();
    mudarFarol(LOW, LOW, LOW); 
    faseAtual = NOITE;
  } 
  else if (faseAtual == NOITE) {
    // A cor aqui é controlada de forma oscilante dentro da função acaoNoite()
    acaoNoite();
  } 
  else if (faseAtual == RESOLUCAO_NOITE) {
    mudarFarol(LOW, LOW, LOW); // Apaga durante o processamento secreto
    resolucaoNoite();
  }
  else if (faseAtual == AMANHECER) {
    mudarFarol(LOW, LOW, LOW); 
    amanhecer();
    checarVitoria();
    
    // Se ninguém ganhou ainda, roda o dia
    if (faseAtual == AMANHECER) {
      Serial.println("O DIA COMECOU! A vila tem 8 minutos para debater...");
      
      // 8 minutos de debate (Farol apagado)
      delay(480000UL); 
      
      Serial.println("A NOITE CAIU!");
      
      // Pisca rápido em amarelo/vermelho por 3 segundos para avisar que a noite chegou
      for(int k=0; k<3; k++) {
        mudarFarol(HIGH, HIGH, LOW); delay(500); // Amarelo
        mudarFarol(HIGH, LOW, LOW);  delay(500); // Vermelho
      }
      
      faseAtual = NOITE; 
    }
  }
  else if (faseAtual == VITORIA_ASSASSINO) {
    Serial.println(">>> FIM DE JOGO: VITORIA DO ASSASSINO! <<<");
    // VERMELHO constante piscando para indicar perigo/vitória do assassino
    for(int i=0; i<10; i++) {
      mudarFarol(HIGH, LOW, LOW); delay(500); 
      mudarFarol(LOW, LOW, LOW);  delay(500);
    }
    faseAtual = SORTEIO; 
  } 
  else if (faseAtual == VITORIA_INOCENTES) {
    Serial.println(">>> FIM DE JOGO: VITORIA DA VILA (INOCENTES)! <<<");
    // AZUL constante piscando para indicar ordem/justiça restaurada
    for(int i=0; i<10; i++) {
      mudarFarol(LOW, LOW, HIGH); delay(500);   
      mudarFarol(LOW, LOW, LOW);  delay(500);   
    }
    faseAtual = SORTEIO; 
  }
}

// --- FUNÇÕES AUXILIARES E LÓGICA DO JOGO ---

void mudarFarol(int r, int g, int b) {
  digitalWrite(FAROL_R, r);
  digitalWrite(FAROL_G, g);
  digitalWrite(FAROL_B, b);
}

// Faz o farol alternar entre Amarelo e Vermelho a cada 1 segundo sem usar delay()
void oscilarFarolNoite() {
  static unsigned long ultimoTempo = 0;
  static bool estadoAmarelo = true;
  unsigned long tempoAtual = millis();

  if (tempoAtual - ultimoTempo >= 1000) { // Altera a cada 1000ms (1 segundo)
    ultimoTempo = tempoAtual;
    estadoAmarelo = !estadoAmarelo;
    
    if (estadoAmarelo) {
      mudarFarol(HIGH, HIGH, LOW); // Amarelo
    } else {
      mudarFarol(HIGH, LOW, LOW);  // Vermelho
    }
  }
}

void iniciarPartida() {
  Serial.println("\n--- SORTEANDO PAPEIS ---");
  mortoAssassino = -1;
  mortoVotacao = -1;
  
  for(int i = 0; i < NUM_JOGADORES; i++) {
    vivo[i] = true;
    papeis[i] = INOCENTE;
    digitalWrite(controles[i].pinoVibracao, LOW); 
  }
  
  idAssassino = random(0, NUM_JOGADORES);
  papeis[idAssassino] = ASSASSINO;
  
  do {
    idDetetive = random(0, NUM_JOGADORES);
  } while (idDetetive == idAssassino);
  papeis[idDetetive] = DETETIVE;

  Serial.print("GABARITO -> Assassino: J"); Serial.print(idAssassino + 1);
  Serial.print(" | Detetive: J"); Serial.println(idDetetive + 1);
  
  // Sorteio de papéis com feedback de vibração
  for (int i = 0; i < NUM_JOGADORES; i++) { 
    int pulsosVibracao = 1; 
    int tempoPulso = 350;
    
    if (papeis[i] == ASSASSINO) {
      pulsosVibracao = 3;  
      tempoPulso = 200;
    }  
    else if (papeis[i] == DETETIVE) {
      pulsosVibracao = 2;  
      tempoPulso = 250;
    } 
    vibrarFeedback(i, pulsosVibracao, tempoPulso);
  }
  delay(1000);
}

int checarCliqueControle(int idJogador) {
  if (digitalRead(controles[idJogador].b1) == LOW) { delay(50); while(digitalRead(controles[idJogador].b1) == LOW); delay(50); return controles[idJogador].id1; }
  if (digitalRead(controles[idJogador].b2) == LOW) { delay(50); while(digitalRead(controles[idJogador].b2) == LOW); delay(50); return controles[idJogador].id2; }
  if (digitalRead(controles[idJogador].b3) == LOW) { delay(50); while(digitalRead(controles[idJogador].b3) == LOW); delay(50); return controles[idJogador].id3; }
  if (digitalRead(controles[idJogador].b4) == LOW) { delay(50); while(digitalRead(controles[idJogador].b4) == LOW); delay(50); return controles[idJogador].id4; }
  if (digitalRead(controles[idJogador].b5) == LOW) { delay(50); while(digitalRead(controles[idJogador].b5) == LOW); delay(50); return controles[idJogador].id5; }
  return -1; 
}

void acaoNoite() {
  Serial.println("A NOITE CAIU! Todos os jogadores vivos votam agora no escuro...");
  
  int jogadoresVivos = 0;
  for(int i = 0; i < NUM_JOGADORES; i++) {
    jaVotou[i] = false;
    alvoEscolhido[i] = -1;
    if (vivo[i]) jogadoresVivos++;
  }

  int votosRecebidos = 0;

  // Enquanto todo mundo não votar, o farol fica oscilando entre Vermelho e Amarelo
  while (votosRecebidos < jogadoresVivos) {
    
    oscilarFarolNoite(); // <--- CHAMA A OSCILAÇÃO DO FAROL AQUI CONSTANTEMENTE
    
    for (int i = 0; i < NUM_JOGADORES; i++) {
      if (vivo[i] && !jaVotou[i]) {
        int alvo = checarCliqueControle(i);
        if (alvo != -1 && vivo[alvo]) { 
          alvoEscolhido[i] = alvo;
          jaVotou[i] = true;
          votosRecebidos++;
          
          vibrarFeedback(i, 1, 80); // Clique tátil de confirmação
          
          Serial.print("J"); Serial.print(i + 1); Serial.println(" registrou sua acao secreta.");
        }
      }
    }
  }
  
  faseAtual = RESOLUCAO_NOITE;
}

void resolucaoNoite() {
  Serial.println("Processando os votos secretos da noite...");
  
  mortoAssassino = -1;
  if (vivo[idAssassino]) {
    mortoAssassino = alvoEscolhido[idAssassino];
  }

  int votosInocentes[NUM_JOGADORES] = {0, 0, 0, 0, 0, 0};
  for(int i = 0; i < NUM_JOGADORES; i++) {
    if (vivo[i] && papeis[i] == INOCENTE) {
      int alvo = alvoEscolhido[i];
      votosInocentes[alvo]++;
    }
  }

  int maisVotos = 0;
  mortoVotacao = -1;
  for (int i = 0; i < NUM_JOGADORES; i++) {
    if (votosInocentes[i] > maisVotos) {
      maisVotos = votosInocentes[i];
      mortoVotacao = i;
    }
  }

  if (vivo[idDetetive]) {
    int alvo = alvoEscolhido[idDetetive];
    int chance = random(1, 101); 
    int numPulsos = 1;
    
    if (chance <= 75) {
      numPulsos = (papeis[alvo] == ASSASSINO) ? 3 : 1; 
    } else {
      numPulsos = (papeis[alvo] == ASSASSINO) ? 1 : 3; 
    }
    
    vibrarFeedback(idDetetive, numPulsos, 250); 
  }

  delay(2000); 
  faseAtual = AMANHECER;
}

void amanhecer() {
  Serial.println("--- RESULTADO DA NOITE ---");
  for(int i=0; i<NUM_JOGADORES; i++) if(vivo[i]) digitalWrite(controles[i].pinoVibracao, LOW);
  delay(500);

  if (mortoAssassino != -1) {
    Serial.print("Vitima do Assassino: J"); Serial.println(mortoAssassino + 1);
    vivo[mortoAssassino] = false;
    
    digitalWrite(controles[mortoAssassino].pinoVibracao, HIGH); 
    delay(4000); 
    digitalWrite(controles[mortoAssassino].pinoVibracao, LOW); 
  }

  if (mortoVotacao != -1 && mortoVotacao != mortoAssassino) {
    Serial.print("Expulso pela votacao da vila: J"); Serial.println(mortoVotacao + 1);
    vivo[mortoVotacao] = false;
    
    digitalWrite(controles[mortoVotacao].pinoVibracao, HIGH);
    delay(4000); 
    digitalWrite(controles[mortoVotacao].pinoVibracao, LOW);
  }
  
  mortoAssassino = -1;
  mortoVotacao = -1;
  delay(1000);
}

void checarVitoria() {
  if (!vivo[idAssassino]) { 
    faseAtual = VITORIA_INOCENTES; 
    return; 
  }
  
  int inocentesVivos = 0;
  for (int i = 0; i < NUM_JOGADORES; i++) {
    if (vivo[i] && papeis[i] != ASSASSINO) inocentesVivos++;
  }
  
  if (inocentesVivos <= 1) {
    faseAtual = VITORIA_ASSASSINO;
  }
}

void vibrarFeedback(int idJogador, int vezes, int tempo) {
  if (!vivo[idJogador]) return; 
  for(int i = 0; i < vezes; i++) {
    digitalWrite(controles[idJogador].pinoVibracao, HIGH); delay(tempo);
    digitalWrite(controles[idJogador].pinoVibracao, LOW);  delay(tempo);
  }
}
