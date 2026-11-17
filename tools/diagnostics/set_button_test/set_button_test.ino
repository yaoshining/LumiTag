// Temporary hardware diagnostic for the TinkerNode SET button (GPIO13 / D3).
constexpr uint8_t kSetButtonPin = 13;
constexpr uint8_t kIndicatorPin = 2;

int lastState;

void setup() {
  Serial.begin(115200);
  pinMode(kSetButtonPin, INPUT_PULLUP);
  pinMode(kIndicatorPin, OUTPUT);
  delay(100);

  lastState = digitalRead(kSetButtonPin);
  digitalWrite(kIndicatorPin, lastState == LOW ? HIGH : LOW);
  Serial.println("SET button diagnostic ready");
  Serial.printf("GPIO13 initial: %s\n", lastState == LOW ? "PRESSED" : "RELEASED");
}

void loop() {
  const int state = digitalRead(kSetButtonPin);
  digitalWrite(kIndicatorPin, state == LOW ? HIGH : LOW);
  if (state != lastState) {
    lastState = state;
    Serial.printf("GPIO13: %s\n", state == LOW ? "PRESSED" : "RELEASED");
    delay(30);  // Debounce the mechanical switch.
  }
}
