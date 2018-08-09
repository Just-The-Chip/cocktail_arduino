class IOtimer {
  private:
    int IO;                 // pin of air vavle
    char jarNum;             // jar number for setting light and color (0 index)
    unsigned long dTimeSet; // mS time that valve was set to on
    unsigned int dOnTime;  // mS time for valve to stay on
    bool dState;            // current on/off state of valve

    unsigned long lTimeSet; // mS time that light was set to on
    unsigned int lOnTime;  // mS time for light to stay on
    bool lState;

    void updateLightState(bool state);

  public:
    IOtimer(int pin, char jar);
    void Update();
    void Set(int microseconds);
    void SetLight(int microseconds);
    void UnsetLight();
};