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
  int pinoFeedback;
};

// Mapeamento dos fios de LED 
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
void piscarFeedback(int idJogador, int vezes, int tempo);
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
    pinMode(controles[i].pinoFeedback, OUTPUT);
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
    mudarFarol(HIGH, HIGH, HIGH); // Branco (Sorteio/Anoitecer inicial)
    iniciarPartida();
    mudarFarol(LOW, LOW, LOW); // Desliga o farol
    faseAtual = NOITE;
  } 
  else if (faseAtual == NOITE) {
    mudarFarol(LOW, LOW, LOW); // Mantém desligado
    acaoNoite();
  } 
  else if (faseAtual == RESOLUCAO_NOITE) {
    mudarFarol(LOW, LOW, LOW); // Mantém desligado
    resolucaoNoite();
  }
  else if (faseAtual == AMANHECER) {
    mudarFarol(LOW, LOW, LOW); // Mantém desligado
    amanhecer();
    checarVitoria();
    
    // Se ninguém ganhou ainda, inicia o cronômetro de 8 minutos
    if (faseAtual == AMANHECER) {
      Serial.println("O DIA COMECOU! A vila tem 8 minutos para debater...");
      
      // 8 minutos = 480.000 milissegundos
      // Mude este valor para testar mais rapido no simulador (ex: 5000 para 5 segundos)
      delay(480000UL); 
      
      Serial.println("A NOITE CAIU!");
      
      // Pisca branco por 3 segundos para avisar que a noite chegou
      mudarFarol(HIGH, HIGH, HIGH);
      delay(3000);
      mudarFarol(LOW, LOW, LOW);
      
      faseAtual = NOITE; 
    }
  }
  else if (faseAtual == VITORIA_ASSASSINO) {
    Serial.println(">>> FIM DE JOGO: VITORIA DO ASSASSINO! <<<");
    for(int i=0; i<8; i++) {
      mudarFarol(HIGH, LOW, LOW); delay(400); // Vermelho
      mudarFarol(LOW, LOW, LOW);  delay(400);
    }
    faseAtual = SORTEIO; 
  } 
  else if (faseAtual == VITORIA_INOCENTES) {
    Serial.println(">>> FIM DE JOGO: VITORIA DA VILA (INOCENTES)! <<<");
    for(int i=0; i<6; i++) {
      mudarFarol(HIGH, LOW, LOW); delay(200);   
      mudarFarol(LOW, HIGH, LOW); delay(200);   
      mudarFarol(LOW, LOW, HIGH); delay(200);   
      mudarFarol(HIGH, HIGH, LOW); delay(200);  
      mudarFarol(HIGH, LOW, HIGH); delay(200);  
      mudarFarol(LOW, HIGH, HIGH); delay(200);  
    }
    mudarFarol(LOW, LOW, LOW);
    faseAtual = SORTEIO; 
  }
}

// --- FUNÇÕES AUXILIARES E LÓGICA DO JOGO ---

void mudarFarol(int r, int g, int b) {
  digitalWrite(FAROL_R, r);
  digitalWrite(FAROL_G, g);
  digitalWrite(FAROL_B, b);
}

void iniciarPartida() {
  Serial.println("\n--- SORTEANDO PAPEIS ---");
  mortoAssassino = -1;
  mortoVotacao = -1;
  
  for(int i = 0; i < NUM_JOGADORES; i++) {
    vivo[i] = true;
    papeis[i] = INOCENTE;
    digitalWrite(controles[i].pinoFeedback, LOW); 
  }
  
  idAssassino = random(0, NUM_JOGADORES);
  papeis[idAssassino] = ASSASSINO;
  
  do {
    idDetetive = random(0, NUM_JOGADORES);
  } while (idDetetive == idAssassino);
  papeis[idDetetive] = DETETIVE;

  Serial.print("GABARITO -> Assassino: J"); Serial.print(idAssassino + 1);
  Serial.print(" | Detetive: J"); Serial.println(idDetetive + 1);
  
  for (int i = 0; i < NUM_JOGADORES; i++) { 
    int piscadas = 1; 
    if (papeis[i] == ASSASSINO) piscadas = 3;  
    else if (papeis[i] == DETETIVE) piscadas = 2; 
    piscarFeedback(i, piscadas, 350);
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

  // Fica travado aqui até TODOS os jogadores vivos apertarem algum botão
  while (votosRecebidos < jogadoresVivos) {
    for (int i = 0; i < NUM_JOGADORES; i++) {
      if (vivo[i] && !jaVotou[i]) {
        int alvo = checarCliqueControle(i);
        if (alvo != -1 && vivo[alvo]) { 
          alvoEscolhido[i] = alvo;
          jaVotou[i] = true;
          votosRecebidos++;
          Serial.print("J"); Serial.print(i + 1); Serial.println(" registrou sua acao secreta.");
        }
      }
    }
  }
  
  faseAtual = RESOLUCAO_NOITE;
}

void resolucaoNoite() {
  Serial.println("Processando os votos secretos da noite...");
  
  // 1. Resolve o ataque do Assassino
  mortoAssassino = -1;
  if (vivo[idAssassino]) {
    mortoAssassino = alvoEscolhido[idAssassino];
  }

  // 2. Resolve os votos dos Inocentes (apenas eles contam para a fogueira)
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

  // 3. Resolve a investigação do Detetive (pisca o LED dele para dar a resposta)
  if (vivo[idDetetive]) {
    int alvo = alvoEscolhido[idDetetive];
    int chance = random(1, 101); 
    int numPiscadas = 1;
    
    if (chance <= 75) {
      // 75% de chance de falar a verdade
      numPiscadas = (papeis[alvo] == ASSASSINO) ? 3 : 1; 
    } else {
      // 25% de chance de mentir
      numPiscadas = (papeis[alvo] == ASSASSINO) ? 1 : 3; 
    }
    
    piscarFeedback(idDetetive, numPiscadas, 300); 
  }

  delay(2000); // Dá um tempinho em silêncio para o detetive entender a resposta
  faseAtual = AMANHECER;
}

void amanhecer() {
  Serial.println("--- RESULTADO DA NOITE ---");
  for(int i=0; i<NUM_JOGADORES; i++) if(vivo[i]) digitalWrite(controles[i].pinoFeedback, LOW);
  delay(500);

  if (mortoAssassino != -1) {
    Serial.print("Vitima do Assassino: J"); Serial.println(mortoAssassino + 1);
    vivo[mortoAssassino] = false;
    digitalWrite(controles[mortoAssassino].pinoFeedback, HIGH); 
    delay(15000); // Fica aceso por 15 segundos
    digitalWrite(controles[mortoAssassino].pinoFeedback, LOW); // Apaga
  }

  // Só anuncia se a vila expulsou alguém e se não foi a mesma pessoa que o Assassino matou
  if (mortoVotacao != -1 && mortoVotacao != mortoAssassino) {
    Serial.print("Expulso pela votacao da vila: J"); Serial.println(mortoVotacao + 1);
    vivo[mortoVotacao] = false;
    digitalWrite(controles[mortoVotacao].pinoFeedback, HIGH);
    delay(15000); // Fica aceso por 15 segundos
    digitalWrite(controles[mortoVotacao].pinoFeedback, LOW); // Apaga
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

void piscarFeedback(int idJogador, int vezes, int tempo) {
  if (!vivo[idJogador]) return; 
  for(int i = 0; i < vezes; i++) {
    digitalWrite(controles[idJogador].pinoFeedback, HIGH); delay(tempo);
    digitalWrite(controles[idJogador].pinoFeedback, LOW);  delay(tempo);
  }
}