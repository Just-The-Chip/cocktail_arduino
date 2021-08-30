class IOtimer {
  private:
    int IO;                 // pin of air vavle
    char jarNum;             // jar number for setting light and color (0 index)
    int ledIndex;             // index of first LED pin
    unsigned long dTimeSet; // mS time that valve was set to on
    long dOnTime;  // mS time for valve to stay on
    bool dState;            // current on/off state of valve

    unsigned long lTimeSet; // mS time that light was set to on
    long lOnTime;  // mS time for light to stay on
    bool lState;

    void updateLightState(bool state);

  public:
    IOtimer(int loadDataPin, int loadClockPin, int jarPin, int ledIndex, char jar);
    void Update();
    void SetPump(long microseconds); //actually miliseconds?
    void SetLight(long microseconds); //I'll figure it out later.
    void UnsetLight();
};