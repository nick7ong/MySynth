#include <iostream>
#include "olcNoiseMaker.h"

using namespace std;

//Freq=>AngVel conversion
double w(double hertz)
{
    return hertz * 2.0 * PI;
}

//Global variables
atomic<double> frequencyOutput = 0.0;
double octaveBaseFrequency = 261.63; //Middle C3
double rootTwelve = pow(2.0, 1.0 / 12);

double Oscillator(double hertz, double time, int type)
{
    switch (type)
    {
    case 0: //Sine Wave
        return sin(w(frequencyOutput) * time);
    case 1: //Square Wave
        return sin(w(frequencyOutput) * time) > 0.0 ? 1.0 : -1.0;
    case 2: //Triangle Wave
        return asin(sin(w(frequencyOutput) * time)) * 2.0 / PI;
    case 3: //Saw Wave
    {
        double output = 0.0;

        for (double i = 1.0; i < 100.0; i++)
            output += (sin(i * w(hertz) * time)) / i;
        
        return output * (2.0 / PI);
    }
    default:
        return 0.0;
    }
}

struct EnvelopeADSR
{
    double attackTime;
    double decayTime;
    double releaseTime;

    double sustainAmplitude;
    double startAmplitude;

    double triggerOnTime;
    double triggerOffTime;

    bool noteOn;

    EnvelopeADSR()
    { //Default envelope ADSR (in seconds)
        attackTime = 0.01;
        decayTime = 0.01;
        startAmplitude = 1.0;
        sustainAmplitude = 0.8;
        releaseTime = 0.02;
        triggerOnTime = 0.0;
        triggerOffTime = 0.0;
    }

    void NoteOn(double timeOn)
    {
        triggerOnTime = timeOn;
        noteOn = true;
    }

    void NoteOff(double timeOff)
    {
        triggerOffTime = timeOff;
        noteOn = false;
    }

    double GetAmplitude(double time)
    {
        double amplitude = 0.0;
        double lifeTime = time - triggerOnTime;

        if (noteOn)
        { //ADS
            //Attack
            if (lifeTime <= attackTime)
                amplitude = (lifeTime / attackTime) * startAmplitude;
            //Decay
            if (lifeTime > attackTime && lifeTime <= (attackTime + decayTime))
                amplitude = ((lifeTime - attackTime) / decayTime) * (sustainAmplitude - startAmplitude) + startAmplitude;
            //Sustain
            if (lifeTime > (attackTime + decayTime))
            {
                amplitude = sustainAmplitude;
            }
        }
        else
        { 
            //Release
            amplitude = ((time - triggerOffTime) / releaseTime) * (0.0 - sustainAmplitude) + sustainAmplitude;
        }
        
        //To stop signals lower than audible hearing from going through
        if (amplitude <= 0.0001)
        {
            amplitude = 0.0;
        }

        return amplitude;
    }
};

EnvelopeADSR envelope;

double MakeNoise(double time)
{
    double output = envelope.GetAmplitude(time) * (+ Oscillator(frequencyOutput * 1.0, time, 3)
                                                   + Oscillator(frequencyOutput * 2.0, time, 0)
                                                   + Oscillator(frequencyOutput * 3.0, time, 2));
    return output * 0.4; //Master Volume
}

int main()
{
    //Get sound hardware
    vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

    //Display output devices
    for (auto d : devices)
        wcout << "Found Output Device" << d << endl;

    //Display visual keyboard C3-C4
    wcout << endl <<
		"|   |   | |   |   |   |   | |   | |   |   |     |" << endl <<
		"|   | W | | E |   |   | T | | Y | | U |   |     |" << endl <<
		"|   |___| |___|   |   |___| |___| |___|   |     |" << endl <<
		"|     |     |     |     |     |     |     |     |" << endl <<
		"|  A  |  S  |  D  |  F  |  G  |  H  |  J  |  K  |" << endl <<
		"|_____|_____|_____|_____|_____|_____|_____|_____|" << endl << endl;


    //Create synth sound class
    olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

    //Link MakeNoise() to sound class
    sound.SetUserFunction(MakeNoise);

    //Add keyboard control
    int currentKey = -1;
    bool keyPressed = false;
    while (1)
    {
        keyPressed = false;
        for (int i = 0; i < 13; i++)
        {
            if (GetAsyncKeyState((unsigned char)("AWSEDFTGYHUJK"[i])) & 0x8000)
            {
                if (currentKey != i)
                {
                    frequencyOutput = octaveBaseFrequency * pow(rootTwelve, i);
                    envelope.NoteOn(sound.GetTime());
                    currentKey = i;
                }
                keyPressed = true;
            }
        }
        if (!keyPressed)
        {
            if (currentKey != -1)
            {
                envelope.NoteOff(sound.GetTime());
                currentKey = -1;
            }
        }
    }

    return 0;
}

