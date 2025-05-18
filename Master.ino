#include <RF24.h>
#include <Servo.h>

// Пины
#define THERMISTOR_PIN A0  // Пин для термистора
#define SERVO_PIN 9        // Пин для сервопривода
#define CE_PIN 7           // Пин CE для NRF24L01
#define CSN_PIN 8          // Пин CSN для NRF24L01

// Константы
#define TEMP_THRESHOLD 25.0  // Порог температуры (°C)
#define BETA 3950            // Бета-коэффициент термистора
#define R0 100000            // Номинальное сопротивление термистора при 25°C
#define R_REF 10000          // Сопротивление резистора в делителе

// Настройка NRF24L01
RF24 radio(CE_PIN, CSN_PIN);
const byte address[2][6] = {"00001", "00002"};  // Адреса для связи

// Настройка сервопривода
Servo servo;

// Переменные
float ownTemp = 0;
float otherTemp = 0;

void setup() {
  Serial.begin(9600);
  pinMode(THERMISTOR_PIN, INPUT);
  
  // Инициализация сервопривода
  servo.attach(SERVO_PIN);
  servo.write(0);  // Форточка закрыта
  
  // Инициализация NRF24L01
  radio.begin();
  radio.openWritingPipe(address[1]);    // Отправка на Узел 2
  radio.openReadingPipe(1, address[0]); // Прием от Узла 2
  radio.setPALevel(RF24_PA_MIN);       // Минимальная мощность для теста
  radio.startListening();
}

void loop() {
  // Измеряем температуру
  ownTemp = readTemperature();
  
  // Отправляем свою температуру Узлу 2
  radio.stopListening();
  radio.write(&ownTemp, sizeof(ownTemp));
  radio.startListening();
  
  // Ждем ответа от Узла 2
  unsigned long startTime = millis();
  bool received = false;
  while (millis() - startTime < 1000) {
    if (radio.available()) {
      radio.read(&otherTemp, sizeof(otherTemp));
      received = true;
      break;
    }
  }
  
  // Вычисляем среднюю температуру
  float averageTemp = (ownTemp + otherTemp) / 2.0;
  
  // Выводим данные
  Serial.print("Master: Собственная температура: ");
  Serial.print(ownTemp);
  Serial.print("°C, Температура Slave: ");
  Serial.print(otherTemp);
  Serial.print("°C, Средняя: ");
  Serial.print(averageTemp);
  Serial.println("°C");
  
  // Управляем сервоприводом
  if (averageTemp > TEMP_THRESHOLD) {
    servo.write(90);  // Открываем форточку
    Serial.println("Master: Температура превышает порог! Открываю форточку.");
  } else {
    servo.write(0);   // Закрываем форточку
  }
  
  delay(5000);  // Ждем 5 секунд
}

float readTemperature() {
  // Читаем аналоговое значение
  int analogValue = analogRead(THERMISTOR_PIN);
  float resistance = R_REF * (1023.0 / analogValue - 1.0);
  
  // Пересчет сопротивления в температуру
  float temp = 1.0 / (1.0 / 298.15 + (1.0 / BETA) * log(resistance / R0));
  temp -= 273.15;  // Перевод из Кельвинов в Цельсий
  return temp;
}