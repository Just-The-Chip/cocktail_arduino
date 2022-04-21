const int startPin = 22;
const int Njars = 21;
const int pumpIO = 43;

void setup() {
  for (int i = startPin; i < (startPin + Njars); i++) {
      digitalWrite(i, LOW);
      pinMode(i, OUTPUT);
  }
  pinMode(pumpIO, OUTPUT);
  digitalWrite(pumpIO, HIGH);
}

void loop() {
    //  for each jar position
    for (int i = startPin; i < (startPin + Njars); i++) {
        //  if first jar, turn off last jar
        if (i == startPin) digitalWrite((startPin + (Njars-1)), LOW);
        //  if any other jar, turn off the previous jar
        else if (i > startPin) digitalWrite(i-1, LOW);
        //  turn on a jar
        digitalWrite(i, HIGH);
        delay(15000);
    }
}