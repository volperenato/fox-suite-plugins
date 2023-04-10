//-------------------------------------------------------------------------------------------------------
//  MisEfx.cpp
//  Reverb plugin based on the Freeverb scheme
//  Customized by Renato Volpe on 07/02/2022.
//
//-------------------------------------------------------------------------------------------------------

#include "MisEfx.h"
#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <math.h>

/*--------------------------------------------------------------------*/
#define MAX_COMB_FILTER_LENGTH_IN_MS 100.0
#define MAX_PREDELAY_VALUE_IN_MS 300.0
#define MAX_AP_FILTER_LENGTH_IN_MS 50.0
#define MAX_REVERB_DECAY_IN_SECONDS 5.0
#define MIN_MOD_RATE_IN_HZ 0.1
#define MAX_MOD_RATE_IN_HZ 10.0
//#define MIN_COMB_FILTER_DELAY_IN_MS 31.1
//#define MAX_COMB_FILTER_DELAY_IN_MS 43.3
//#define MIN_AP_FILTER_DELAY_IN_MS 1.1
//#define MAX_AP_FILTER_DELAY_IN_MS 4.7
#define STEREO_SPREAD_COEFFICIENT_IN_MS 1.0
#define NUM_PRESETS 5
/*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*/
// Reverb class constructor
MisEfx::MisEfx(audioMasterCallback audioMaster)
    : AudioEffectX(audioMaster, NUM_PRESETS, Param_Count) // n program, n parameters
{
    setNumInputs(2);		// stereo in
    setNumOutputs(2);		// stereo out
    setUniqueID('vMis');	// identify    
    InitPlugin();
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
    return new MisEfx(audioMaster);
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// Initialize all the objects and parameters
void MisEfx::InitPlugin()
{
    /*.......................................*/
    // initialize reverb plug-in parameters
    InitPresets();

    // get current sample rate
    int currSampleRate = getSampleRate();

    /*.......................................*/
    // init Reverb
    Reverb = new Freeverb();
    Reverb->init(currSampleRate, rev_wet, rev_decay, rev_damping, rev_smearing, rev_spread, rev_preDelay);

    /*.......................................*/
    // init Output LPF filter
    outputLPF = new LPFButterworth;
    outputLPF->init(currSampleRate);
    outputLPF->setCutoffFrequency(rev_lpfFreq);

    /*.......................................*/
    // init Output HPF filter
    outputHPF = new HPFButterworth;
    outputHPF->init(currSampleRate);
    outputHPF->setCutoffFrequency(rev_hpfFreq);

    /*.......................................*/
    // init Chorus
   /* chorus = new ModDelay;
    ModulationType modType = ModulationType::Chorus;
    chorus->init(currSampleRate, modType, rev_modRate, rev_modDepth);*/

    /*.......................................*/
    // init tremolo
    tremolo = new Tremolo;
    modWaveform = OscillatorType::Sine;
    tremolo->init(currSampleRate, modWaveform, rev_modRate, rev_modDepth);
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// replace the "setSampleRate" method with user-defined one
void MisEfx::setSampleRate(float sampleRate)
{
    // Call AudioEffect "setSampleRate" method
    AudioEffect::setSampleRate(sampleRate);

    // Call setSampleRate on every needed module
    Reverb->setSampleRate(sampleRate);
    tremolo->setSampleRate(sampleRate);
    outputLPF->setSampleRate(sampleRate);
    outputHPF->setSampleRate(sampleRate);
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// Define presets parameters values
void MisEfx::InitPresets()
{
    rev_presets = new ReverbPresets[NUM_PRESETS];

    /*----------------------------------------------------*/
    // "Default" preset
    strcpy(rev_presets[0].name, "Default");
    rev_presets[0].rev_wet = 0.2;                        // given as a number between 0 and 1
    rev_presets[0].rev_decay = 1.0;                      // given in seconds
    rev_presets[0].rev_smearing = 0.7;                   // given as a number between 0 and 1
    rev_presets[0].rev_damping = 0.5;                    // given as a number between 0 and 1
    rev_presets[0].rev_lpfFreq = MAX_LPF_FREQUENCY;      // given as a frequency
    rev_presets[0].rev_hpfFreq = MIN_HPF_FREQUENCY;      // given as a frequency
    rev_presets[0].rev_preDelay = 10;                    // given in milliseconds
    rev_presets[0].rev_modRate = 1.0;                    // given in hz
    rev_presets[0].rev_modDepth = 0.3;                   // given as a number between 0 and 1
    rev_presets[0].rev_spread = 0.3;                     // given as a number between 0 and 1

    /*----------------------------------------------------*/
    // "Dreamy" preset
    strcpy(rev_presets[1].name, "Dreamy");
    rev_presets[1].rev_wet = 0.5;
    rev_presets[1].rev_decay = 3.3;
    rev_presets[1].rev_smearing = 0.8;
    rev_presets[1].rev_damping = 0.6;
    rev_presets[1].rev_lpfFreq = 17000.0;
    rev_presets[1].rev_hpfFreq = 200.0;
    rev_presets[1].rev_preDelay = 80;
    rev_presets[1].rev_modRate = 1.0;
    rev_presets[1].rev_modDepth = 0.6;
    rev_presets[1].rev_spread = 1.0;

    /*----------------------------------------------------*/
    // "Short" preset
    strcpy(rev_presets[2].name, "Short");
    rev_presets[2].rev_wet = 0.2;
    rev_presets[2].rev_decay = 2;
    rev_presets[2].rev_smearing = 0.5;
    rev_presets[2].rev_damping = 0.4;
    rev_presets[2].rev_lpfFreq = MAX_LPF_FREQUENCY;
    rev_presets[2].rev_hpfFreq = MIN_HPF_FREQUENCY;
    rev_presets[2].rev_preDelay = 20;
    rev_presets[2].rev_modRate = 1.0;
    rev_presets[2].rev_modDepth = 0.1;
    rev_presets[2].rev_spread = 0.2;

    /*----------------------------------------------------*/
    // "Metallic" preset
    strcpy(rev_presets[3].name, "Metallic");
    rev_presets[3].rev_wet = 0.5;
    rev_presets[3].rev_decay = 2.2;
    rev_presets[3].rev_smearing = 0.0;
    rev_presets[3].rev_damping = 0.0;
    rev_presets[3].rev_lpfFreq = MAX_LPF_FREQUENCY;
    rev_presets[3].rev_hpfFreq = 650.0;
    rev_presets[3].rev_preDelay = 20;
    rev_presets[3].rev_modRate = 0.1;
    rev_presets[3].rev_modDepth = 0.0;
    rev_presets[3].rev_spread = 1.0;

    /*----------------------------------------------------*/
    // "Wobbly" preset
    strcpy(rev_presets[4].name, "Wobbly");
    rev_presets[4].rev_wet = 0.65;
    rev_presets[4].rev_decay = 2.0;
    rev_presets[4].rev_smearing = 0.7;
    rev_presets[4].rev_damping = 0.3;
    rev_presets[4].rev_lpfFreq = 15000.0;
    rev_presets[4].rev_hpfFreq = 500.0;
    rev_presets[4].rev_preDelay = 20;
    rev_presets[4].rev_modRate = 3.0;
    rev_presets[4].rev_modDepth = 1.0;
    rev_presets[4].rev_spread = 0.3;

    // Set the program when creating a new plugin instance
    int initIdx = 0;
    AudioEffect::setProgram(initIdx);
    rev_wet = rev_presets[initIdx].rev_wet;
    rev_decay = rev_presets[initIdx].rev_decay;
    rev_smearing = rev_presets[initIdx].rev_smearing;
    rev_damping = rev_presets[initIdx].rev_damping;
    rev_lpfFreq = rev_presets[initIdx].rev_lpfFreq;
    rev_hpfFreq = rev_presets[initIdx].rev_hpfFreq;
    rev_preDelay = rev_presets[initIdx].rev_preDelay;
    rev_modRate = rev_presets[initIdx].rev_modRate;
    rev_modDepth = rev_presets[initIdx].rev_modDepth;
    rev_spread = rev_presets[initIdx].rev_spread;

}
/*--------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------------------------------
  -------------------------------------  PROCESS REPLACING  ---------------------------------------------------
  ------------------------------------------------------------------------------------------------------------ */
void MisEfx::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
    // Extract input and output buffers
    float* inL = inputs[0]; // buffer input left
    float* inR = inputs[1]; // buffer input right

    float* outL = outputs[0]; // buffer output left
    float* outR = outputs[1]; // buffer output right

    // Cycle over the sample frames number
    for (int i = 0; i < sampleFrames; i++) {

        // Create arrays for Reverb processing
        float rev_inputs[2] = { inL[i],  inR[i] };
        float rev_outputs[2] = { 0.0, 0.0 };

        // Process Reverb
        Reverb->processAudio(rev_inputs, rev_outputs);

        // Unpack output array
        float outputL = rev_outputs[0];
        float outputR = rev_outputs[1];

        // Chorus 
        //outputL = chorus->processAudio(outputL);
        //outputR = chorus->processAudio(outputR);        

        // Output LPF processing
        outputL = outputLPF->processAudio(outputL);
        outputR = outputLPF->processAudio(outputR);

        // Output HPF processing
        outputL = outputHPF->processAudio(outputL);
        outputR = outputHPF->processAudio(outputR);

        //// Tremolo processing
        outputL = tremolo->processAudio(outputL);
        outputR = tremolo->processAudio(outputR);

        // Stereo spread processing + output allocation
        outL[i] = outputL;
        outR[i] = outputR;
    }
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// Convert a value from interval [minValue, maxValue] to [0,1]
float MisEfx::mapValueIntoRange(float value, float minvalue, float maxValue)
{
    return minvalue + value * (maxValue - minvalue);
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// Convert a value from interval [0,1] to [minValue, maxValue]
float MisEfx::mapValueOutsideRange(float value, float minValue, float maxValue)
{
    return (value - minValue) / (maxValue - minValue);
}
/*--------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------------------------------
  ------------------------------------------  PARAMETERS  ------------------------------------------------------
  ------------------------------------------------------------------------------------------------------------ */
  // set reverb parameters values
void MisEfx::setParameter(VstInt32 index, float value)
{
    switch (index) {
    case Param_wet:
    {
        rev_wet = value;
        Reverb->setReverbWet(rev_wet);
        break;
    }
    case Param_decay:
    {
        rev_decay = value * MAX_REVERB_DECAY_IN_SECONDS;
        Reverb->setReverbDecayInSeconds(rev_decay);
        break;
    }
    case Param_smearing:
    {
        rev_smearing = value;
        Reverb->setReverbSmearing(rev_smearing);
        break;
    }
    case Param_damping:
    {
        rev_damping = value;
        float dampingFrequency = mapValueIntoRange(1.0 - rev_damping, MIN_LPF_FREQUENCY, MAX_LPF_FREQUENCY);
        Reverb->setReverbDampingFrequency(dampingFrequency);
        break;
    }
    case Param_lpfFreq:
    {
        rev_lpfFreq = exp(mapValueIntoRange(value, MIN_LPF_FREQUENCY_LOG, MAX_LPF_FREQUENCY_LOG));
        outputLPF->setCutoffFrequency(rev_lpfFreq);
        break;
    }
    case Param_hpfFreq:
    {
        rev_hpfFreq = exp(mapValueIntoRange(value, MIN_HPF_FREQUENCY_LOG, MAX_HPF_FREQUENCY_LOG));
        outputHPF->setCutoffFrequency(rev_hpfFreq);
        break;
    }
    case Param_preDelay:
    {
        rev_preDelay = value * MAX_PREDELAY_VALUE_IN_MS;
        Reverb->setReverbPreDelayInMilliseconds(rev_preDelay);
        break;
    }
    case Param_ModRate:
    {
        rev_modRate = mapValueIntoRange(value, MIN_MOD_RATE_IN_HZ, MAX_MOD_RATE_IN_HZ);
        //chorus->setModRate(rev_modRate);
        tremolo->setModRate(rev_modRate);
        /*for (int i = 0; i < NUM_ALLPASS_FILTERS_IN; i++) {
            apFiltersL_input[i].setModRate(rev_modRate);
            apFiltersR_input[i].setModRate(rev_modRate);
        }*/
        /*for (int i = 0; i < NUM_ALLPASS_FILTERS_OUT; i++) {
            apFiltersL_output[i].setModRate(rev_modRate);
            apFiltersR_output[i].setModRate(rev_modRate);
        }*/
        break;
    }
    case Param_ModDepth:
    {
        rev_modDepth = value;
        //chorus->setModDepth(rev_modDepth);
        tremolo->setModDepth(rev_modDepth);
        /*for (int i = 0; i < NUM_ALLPASS_FILTERS_IN; i++) {
            apFiltersL_input[i].setModDepth(rev_modDepth);
            apFiltersR_input[i].setModDepth(rev_modDepth);
        }*/
        /*for (int i = 0; i < NUM_ALLPASS_FILTERS_OUT; i++) {
            apFiltersL_output[i].setModDepth(rev_modDepth);
            apFiltersR_output[i].setModDepth(rev_modDepth);
        }*/
        break;
    }
    case Param_spread:
    {
        rev_spread = value;
        Reverb->setReverbSpread(rev_spread);
        break;
    }
    default:
        break;
    }
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// return reverb parameters values
float MisEfx::getParameter(VstInt32 index)
{
    float param = 0;
    switch (index) {
    case Param_wet:
    {
        param = rev_wet;
        break;
    }
    case Param_decay:
    {
        param = rev_decay / MAX_REVERB_DECAY_IN_SECONDS;
        break;
    }
    case Param_smearing:
    {
        param = rev_smearing;
        break;
    }
    case Param_damping:
    {
        param = rev_damping;
        break;
    }
    case Param_lpfFreq:
    {
        param = mapValueOutsideRange(log(rev_lpfFreq), MIN_LPF_FREQUENCY_LOG, MAX_LPF_FREQUENCY_LOG);
        break;
    }
    case Param_hpfFreq:
    {
        param = mapValueOutsideRange(log(rev_hpfFreq), MIN_HPF_FREQUENCY_LOG, MAX_HPF_FREQUENCY_LOG);
        break;
    }
    case Param_preDelay:
    {
        param = rev_preDelay / MAX_PREDELAY_VALUE_IN_MS;
        break;
    }
    case Param_ModRate:
    {
        param = mapValueOutsideRange(rev_modRate, MIN_MOD_RATE_IN_HZ, MAX_MOD_RATE_IN_HZ);
        break;
    }
    case Param_ModDepth:
    {
        param = rev_modDepth;
        break;
    }
    case Param_spread:
    {
        param = rev_spread;
        break;
    }
    default:
        break;
    }
    return param;
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// return reverb parameters labels
void MisEfx::getParameterLabel(VstInt32 index, char* label)
{
    switch (index) {
    case Param_wet:
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    case Param_decay:
        vst_strncpy(label, "s", kVstMaxParamStrLen);
    case Param_smearing:
        vst_strncpy(label, "dB", kVstMaxParamStrLen);
        break;
    case Param_damping:
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    case Param_lpfFreq:
        vst_strncpy(label, "Hz", kVstMaxParamStrLen);
        break;
    case Param_hpfFreq:
        vst_strncpy(label, "Hz", kVstMaxParamStrLen);
        break;
    case Param_preDelay:
        vst_strncpy(label, "ms", kVstMaxParamStrLen);
        break;
    case Param_ModRate:
        vst_strncpy(label, "Hz", kVstMaxParamStrLen);
        break;
    case Param_ModDepth:
        vst_strncpy(label, "%", kVstMaxParamStrLen);
        break;
    case Param_spread:
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    default:
        break;
    }
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// return reverb parameters values for displaying purposes
void MisEfx::getParameterDisplay(VstInt32 index, char* text)
{
    switch (index) {
    case Param_wet:
        float2string(rev_wet * 10, text, kVstMaxParamStrLen);
        break;
    case Param_decay:
        float2string(rev_decay, text, kVstMaxParamStrLen);
        break;
    case Param_smearing:
        float2string(rev_smearing * 10, text, kVstMaxParamStrLen);
        break;
    case Param_damping:
        float2string(rev_damping * 10, text, kVstMaxParamStrLen);
        break;
    case Param_lpfFreq:
        float2string(rev_lpfFreq, text, kVstMaxParamStrLen);
        break;
    case Param_hpfFreq:
        float2string(rev_hpfFreq, text, kVstMaxParamStrLen);
        break;
    case Param_preDelay:
        float2string(rev_preDelay, text, kVstMaxParamStrLen);
        break;
    case Param_ModRate:
        float2string(rev_modRate, text, kVstMaxParamStrLen);
        break;
    case Param_ModDepth:
        float2string(rev_modDepth * 100, text, kVstMaxParamStrLen);
        break;
    case Param_spread:
        float2string(rev_spread * 10, text, kVstMaxParamStrLen);
        break;
    default:
        break;
    }
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// return the parameter's name for displaying purpose
void MisEfx::getParameterName(VstInt32 index, char* text)
{
    switch (index) {
    case Param_wet:
        vst_strncpy(text, "Wet", kVstMaxParamStrLen);
        break;
    case Param_decay:
        vst_strncpy(text, "Decay", kVstMaxParamStrLen);
        break;
    case Param_smearing:
        vst_strncpy(text, "Smear", kVstMaxParamStrLen);
        break;
    case Param_damping:
        vst_strncpy(text, "Damp", kVstMaxParamStrLen);
        break;
    case Param_lpfFreq:
        vst_strncpy(text, "LPF Freq", kVstMaxParamStrLen);
        break;
    case Param_hpfFreq:
        vst_strncpy(text, "HPF Freq", kVstMaxParamStrLen);
        break;
    case Param_preDelay:
        vst_strncpy(text, "PreDelay", kVstMaxParamStrLen);
        break;
    case Param_ModRate:
        vst_strncpy(text, "ModRate", kVstMaxParamStrLen);
        break;
    case Param_ModDepth:
        vst_strncpy(text, "ModDepth", kVstMaxParamStrLen);
        break;
    case Param_spread:
        vst_strncpy(text, "Spread", kVstMaxParamStrLen);
        break;
    default:
        break;
    }
}
/*--------------------------------------------------------------------*/


/* ------------------------------------------------------------------------------------------------------------
 -------------------------------------------  PROGRAM  --------------------------------------------------------
 ------------------------------------------------------------------------------------------------------------ */
void MisEfx::setProgram(VstInt32 program)
{
    // Call the current implementation of "setProgram"
    AudioEffect::setProgram(program);

    // Create an instante of ReverbPresets with current preset
    ReverbPresets* cp = &rev_presets[curProgram];

    // Set each parameter
    setParameter(Param_wet, cp->rev_wet);
    setParameter(Param_decay, cp->rev_decay / MAX_REVERB_DECAY_IN_SECONDS);
    setParameter(Param_damping, cp->rev_damping);
    setParameter(Param_smearing, cp->rev_smearing);
    setParameter(Param_spread, cp->rev_spread);
    setParameter(Param_ModDepth, cp->rev_modDepth);
    setParameter(Param_preDelay, cp->rev_preDelay / MAX_PREDELAY_VALUE_IN_MS);
    setParameter(Param_lpfFreq, mapValueOutsideRange(log(cp->rev_lpfFreq), MIN_LPF_FREQUENCY_LOG, MAX_LPF_FREQUENCY_LOG));
    setParameter(Param_hpfFreq, mapValueOutsideRange(log(cp->rev_hpfFreq), MIN_HPF_FREQUENCY_LOG, MAX_HPF_FREQUENCY_LOG));
    setParameter(Param_ModRate, mapValueOutsideRange(cp->rev_modRate, MIN_MOD_RATE_IN_HZ, MAX_MOD_RATE_IN_HZ));
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// return program's current name
void MisEfx::getProgramName(char* name)
{
    strcpy(name, rev_presets[curProgram].name);
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// get the program's list
bool MisEfx::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text)
{
    if (index < NUM_PRESETS)
    {
        strcpy(text, rev_presets[index].name);
        return true;
    }
    return false;
}
/*--------------------------------------------------------------------*/


/* ------------------------------------------------------------------------------------------------------------
 ---------------------------------------------  NAME  ---------------------------------------------------------
 ------------------------------------------------------------------------------------------------------------ */

 /*--------------------------------------------------------------------*/
bool MisEfx::getEffectName(char* name)
{
    vst_strncpy(name, "Reverb", kVstMaxEffectNameLen);
    return true;
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
bool MisEfx::getVendorString(char* name)
{
    vst_strncpy(name, "Fox Suite", kVstMaxVendorStrLen);
    return true;
}
/*--------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------------------------------
 ------------------------------------------  DESTRUCTOR  ------------------------------------------------------
 ------------------------------------------------------------------------------------------------------------ */
MisEfx::~MisEfx()
{
    //Free Reverb
    Reverb->~Freeverb();

    // destroy chorus
    //chorus->~ModDelay();

    // destroy lpf
    outputLPF->~LPFButterworth();

    // destroy hpf
    outputHPF->~HPFButterworth();

    // destroy tremolo
    tremolo->~Tremolo();
}


