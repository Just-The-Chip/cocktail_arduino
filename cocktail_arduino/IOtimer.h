class IOtimer {
  private:
    int IO;                 // pin of air vavle
    char jarNum;             // jar number for setting light and color (0 index)
    int ledIndex;             // index of first LED pin

  public:
    IOtimer(int jarPin, char jar);
    void SetValve();
    void UnsetValve();
};