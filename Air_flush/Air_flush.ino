int startPin = 22;
int Njars = 21;

void setup() {
  // put your setup code here, to run once:
    for (int i = startPin; i < (startPin + Njars); i++) {
        digitalWrite(i, LOW);
        pinMode(i, OUTPUT);
    }
}

void loop() {
  // put your main code here, to run repeatedly:
    for (int i = startPin; i < (startPin + Njars); i++) {
        digitalWrite(i, HIGH);
        delay(15000);
    }
}