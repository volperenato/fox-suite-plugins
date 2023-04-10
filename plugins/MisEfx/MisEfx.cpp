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
#include "constants.h"

/*--------------------------------------------------------------------*/
#define NUM_PRESETS 1
#define DEFAULT_NUMBER_OF_INTERNAL_CHANNELS_FDN	16
#define DIFFUSER_DELAY_BUFFER_SIZE_MS 2000.0
#define FEEDBACK_DELAY_BUFFER_SIZE_MS 2000.0
#define MAX_REVERB_DECAY_IN_SECONDS 30.0
#define MIN_FEEDBACK_DELAY_LENGTH 100.0
#define NUMBER_OF_DIFFUSION_STEPS 5
#define EARLYREFL_DELAY_DISTRIBUTION DelayDistribution::RandomInRange
#define DIFFUSER_DELAY_DISTRIBUTION DelayDistribution::RandomInRange
#define FEEDBACK_DELAY_DISTRIBUTION DelayDistribution::RandomInRange
#define OUTPUT_LPF_TYPE LPFilterType::Shelving
#define DAMPING_LPF_TYPE LPFilterType::Vicanek
#define OUTPUT_HPF_TYPE HPFilterType::Shelving
#define DIFFUSION_LOGIC DiffuserDelayLogic::Doubled
#define MIN_DAMPING_FREQUENCY 200.0
#define MAX_DAMPING_FREQUENCY 20000.0
#define MAX_MOD_RATE 5.0
#define LPF_FILTER_MIN_FREQ 100.0
#define HPF_FILTER_MIN_FREQ 40.0
#define LPF_FILTER_MAX_FREQ 20000.0
#define HPF_FILTER_MAX_FREQ 7000.0
/*--------------------------------------------------------------------*/

const float MIN_DAMPING_FREQUENCY_LOG = log(MIN_DAMPING_FREQUENCY);
const float MAX_DAMPING_FREQUENCY_LOG = log(MAX_DAMPING_FREQUENCY);
const float LPF_FILTER_MAX_FREQ_LOG = log(LPF_FILTER_MAX_FREQ);
const float LPF_FILTER_MIN_FREQ_LOG = log(LPF_FILTER_MIN_FREQ);

/*--------------------------------------------------------------------*/
// Reverb class constructor
Feedverb::Feedverb(audioMasterCallback audioMaster)
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
    return new Feedverb(audioMaster);
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// Initialize all the objects and parameters
void Feedverb::InitPlugin()
{
    /*.......................................*/
    // initialize reverb plug-in parameters
    //InitPresets();

    // get current sample rate
    int sampleRate = getSampleRate();

    fdnver_modFeed = 0.4;
    fdnver_modRate = 0.0;
    fdnver_modDepth = 0.0;
    fdnver_mix = 0.5;
    updateMix();
    fdnver_roomSize = 0.5;
    fdnver_highfreq = 0.6;    
    fdnver_modMix = 0.5;
    fdnver_spread = 1.0;
    fdnver_freqDamp = 0.5;
    fdnver_lowfreq = MAX_LPF_FREQUENCY;
    fdnver_stereoSpread = 0.5;
    fdnver_decay = 0.2;
    fdnver_lpftype = 0.61;
    fdnver_highfreq = MIN_HPF_FREQUENCY;

    /*.......................................*/
    // Create FDN objects
    fdnver_FDN = new FDN(2, DEFAULT_NUMBER_OF_INTERNAL_CHANNELS_FDN, 2, NUMBER_OF_DIFFUSION_STEPS, 1);

    // Initialize objects (allocate delay lines)
    fdnver_FDN->initialize(DIFFUSER_DELAY_BUFFER_SIZE_MS, FEEDBACK_DELAY_BUFFER_SIZE_MS, sampleRate);

    // Set room size
    fdnver_FDN->setRoomSize(fdnver_roomSize, DIFFUSION_LOGIC, DIFFUSER_DELAY_DISTRIBUTION, FEEDBACK_DELAY_DISTRIBUTION);

    // Set decay
    fdnver_FDN->setDecayInSeconds(fdnver_decay * MAX_REVERB_DECAY_IN_SECONDS);

    // set damping frequency
    fdnver_FDN->setDampingFrequency(exp(mapValueIntoRange(1.0 - fdnver_freqDamp, MIN_DAMPING_FREQUENCY_LOG, MAX_DAMPING_FREQUENCY_LOG)));
    fdnver_FDN->setDampingType(DAMPING_LPF_TYPE);

    // set output low & high pass filters
    fdnver_FDN->setLowPassFrequency(fdnver_lowfreq);
    fdnver_FDN->setLowPassType(OUTPUT_LPF_TYPE);
    fdnver_FDN->setHighPassType(OUTPUT_HPF_TYPE);
    fdnver_FDN->setHighPassFrequency(fdnver_highfreq);

    // Modulation
    fdnver_FDN->setModDepth(fdnver_modDepth);
    fdnver_FDN->setModRate(fdnver_modRate);

    // stereo spread
    fdnver_FDN->setStereoSpread(fdnver_stereoSpread);

    // output mixing mode
    fdnver_FDN->setMixMode(MixMode::First);    
}

/*--------------------------------------------------------------------*/

void Feedverb::updateMix() {
    _wet = sin(fdnver_mix * M_PI * 0.5);
    _dry = cos(fdnver_mix * M_PI * 0.5);
}

/*--------------------------------------------------------------------*/
// replace the "setSampleRate" method with user-defined one
void Feedverb::setSampleRate(float sampleRate)
{
    // Call AudioEffect "setSampleRate" method
    AudioEffect::setSampleRate(sampleRate);

    // Call setSampleRate on every needed module
    fdnver_FDN->setSampleRate(sampleRate);
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// Define presets parameters values
//void Feedverb::InitPresets()
//{
//    rev_presets = new ReverbPresets[NUM_PRESETS];
//
//    /*----------------------------------------------------*/
//    // "Default" preset
//    strcpy(rev_presets[0].name, "Default");
//    rev_presets[0].rev_wet = 0.2;                        // given as a number between 0 and 1
//    rev_presets[0].rev_decay = 1.0;                      // given in seconds
//    rev_presets[0].rev_smearing = 0.7;                   // given as a number between 0 and 1
//    rev_presets[0].rev_damping = 0.5;                    // given as a number between 0 and 1  
//    rev_presets[0].rev_preDelay = 10;                    // given in milliseconds
//    rev_presets[0].rev_modRate = 1.0;                    // given in hz
//    rev_presets[0].rev_modDepth = 0.3;                   // given as a number between 0 and 1
//    rev_presets[0].rev_spread = 0.3;                     // given as a number between 0 and 1
//
//    /*----------------------------------------------------*/
//    // "Dreamy" preset
//    strcpy(rev_presets[1].name, "Dreamy");
//    rev_presets[1].rev_wet = 0.5;
//    rev_presets[1].rev_decay = 3.3;
//    rev_presets[1].rev_smearing = 0.8;
//    rev_presets[1].rev_damping = 0.6;
//    rev_presets[1].rev_lpfFreq = 17000.0;
//    rev_presets[1].rev_hpfFreq = 200.0;
//    rev_presets[1].rev_preDelay = 80;
//    rev_presets[1].rev_modRate = 1.0;
//    rev_presets[1].rev_modDepth = 0.6;
//    rev_presets[1].rev_spread = 1.0;
//
//    /*----------------------------------------------------*/
//    // "Short" preset
//    strcpy(rev_presets[2].name, "Short");
//    rev_presets[2].rev_wet = 0.2;
//    rev_presets[2].rev_decay = 2;
//    rev_presets[2].rev_smearing = 0.5;
//    rev_presets[2].rev_damping = 0.4;
//    rev_presets[2].rev_preDelay = 20;
//    rev_presets[2].rev_modRate = 1.0;
//    rev_presets[2].rev_modDepth = 0.1;
//    rev_presets[2].rev_spread = 0.2;
//
//    /*----------------------------------------------------*/
//    // "Metallic" preset
//    strcpy(rev_presets[3].name, "Metallic");
//    rev_presets[3].rev_wet = 0.5;
//    rev_presets[3].rev_decay = 2.2;
//    rev_presets[3].rev_smearing = 0.0;
//    rev_presets[3].rev_damping = 0.0;
//    rev_presets[3].rev_lpfFreq = MAX_LPF_FREQUENCY;
//    rev_presets[3].rev_hpfFreq = 650.0;
//    rev_presets[3].rev_preDelay = 20;
//    rev_presets[3].rev_modRate = 0.1;
//    rev_presets[3].rev_modDepth = 0.0;
//    rev_presets[3].rev_spread = 1.0;
//
//    /*----------------------------------------------------*/
//    // "Wobbly" preset
//    strcpy(rev_presets[4].name, "Wobbly");
//    rev_presets[4].rev_wet = 0.65;
//    rev_presets[4].rev_decay = 2.0;
//    rev_presets[4].rev_smearing = 0.7;
//    rev_presets[4].rev_damping = 0.3;
//    rev_presets[4].rev_lpfFreq = 15000.0;
//    rev_presets[4].rev_hpfFreq = 500.0;
//    rev_presets[4].rev_preDelay = 20;
//    rev_presets[4].rev_modRate = 3.0;
//    rev_presets[4].rev_modDepth = 1.0;
//    rev_presets[4].rev_spread = 0.3;
//
//    // Set the program when creating a new plugin instance
//    int initIdx = 0;
//    AudioEffect::setProgram(initIdx);
//    rev_wet = rev_presets[initIdx].rev_wet;
//    rev_decay = rev_presets[initIdx].rev_decay;
//    rev_smearing = rev_presets[initIdx].rev_smearing;
//    rev_damping = rev_presets[initIdx].rev_damping;
//    rev_lpfFreq = rev_presets[initIdx].rev_lpfFreq;
//    rev_hpfFreq = rev_presets[initIdx].rev_hpfFreq;
//    rev_preDelay = rev_presets[initIdx].rev_preDelay;
//    rev_modRate = rev_presets[initIdx].rev_modRate;
//    rev_modDepth = rev_presets[initIdx].rev_modDepth;
//    rev_spread = rev_presets[initIdx].rev_spread;
//
//}
/*--------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------------------------------
  -------------------------------------  PROCESS REPLACING  ---------------------------------------------------
  ------------------------------------------------------------------------------------------------------------ */
void Feedverb::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
    // Extract input and output buffers
    float* inL = inputs[0]; // buffer input left
    float* inR = inputs[1]; // buffer input right

    float* outL = outputs[0]; // buffer output left
    float* outR = outputs[1]; // buffer output right
    
    // Cycle over the sample frames number
    for (int i = 0; i < sampleFrames; i++) {      
        float input[2]  = { inL[i], inR[i] };
        float output[2] = {0.0, 0.0};

        fdnver_FDN->processAudio(input, output);
        //float yn = chorus->processAudio(inL[i]);
        //output[0] = modDel->processAudio(inL[i]);
        //output[1] = modDel->processAudio(inR[i]);

        outL[i] = output[0] * _wet + inL[i] * _dry;
        outR[i] = output[1] * _wet + inR[i] * _dry;
        //outL[i] = yn * _wet + inL[i] * _dry;
        //outR[i] = yn * _wet + inR[i] * _dry;
    }
}
/*--------------------------------------------------------------------*/


/* ------------------------------------------------------------------------------------------------------------
  ------------------------------------------  PARAMETERS  ------------------------------------------------------
  ------------------------------------------------------------------------------------------------------------ */
  // set reverb parameters values
void Feedverb::setParameter(VstInt32 index, float value)
{
    switch (index) {
    case Param_mix: {
        fdnver_mix = value;   
        updateMix();
        break;
    }    
    case Param_roomSize: {
        fdnver_roomSize = value;
        fdnver_FDN->setRoomSize(fdnver_roomSize, DIFFUSION_LOGIC, DIFFUSER_DELAY_DISTRIBUTION, FEEDBACK_DELAY_DISTRIBUTION);
        break;
    }
    case Param_decay: {
        fdnver_decay = value;
        fdnver_FDN->setDecayInSeconds(fdnver_decay * MAX_REVERB_DECAY_IN_SECONDS);
        break;
    }
    case Param_spread: {
        fdnver_spread = value;
        fdnver_FDN->setStereoSpread(value);
        break;
    }
    case Param_modDepth: {
        fdnver_modDepth = value;
        fdnver_FDN->setModDepth(fdnver_modDepth);
        break;
    }
    case Param_modRate: {
        fdnver_modRate = value;
        fdnver_FDN->setModRate(fdnver_modRate * MAX_MOD_RATE);
        break;
    }
    case Param_freqDamp: { 
        fdnver_freqDamp = value;
        //float freq = mapValueIntoRange(1.0 - fdnver_freqDamp, MIN_DAMPING_FREQUENCY, MAX_DAMPING_FREQUENCY);
        float freq = exp(mapValueIntoRange(1.0 - fdnver_freqDamp, MIN_DAMPING_FREQUENCY_LOG, MAX_DAMPING_FREQUENCY_LOG));
        fdnver_FDN->setDampingFrequency(freq);
        break;
    }
    case Param_lpf: {
        fdnver_lowfreq = exp(mapValueIntoRange(value, LPF_FILTER_MIN_FREQ_LOG, LPF_FILTER_MAX_FREQ_LOG));
        fdnver_FDN->setLowPassFrequency(fdnver_lowfreq);
        break;
    }
    case Param_hpf: {
        //fdnver_highfreq = exp(mapValueIntoRange(value, MIN_HPF_FREQUENCY_LOG, MAX_HPF_FREQUENCY_LOG));
        fdnver_highfreq = mapValueIntoRange(value, HPF_FILTER_MIN_FREQ, HPF_FILTER_MAX_FREQ);
        fdnver_FDN->setHighPassFrequency(fdnver_highfreq);
        break;
    }    
    default:
        break;
    }
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// return reverb parameters values
float Feedverb::getParameter(VstInt32 index)
{
    float param = 0;
    switch (index) {
    case Param_mix: {
        param = fdnver_mix;
        break;
    }   
    case Param_modDepth: {
        param = fdnver_modDepth;
        break;
    }
    case Param_modRate: {
        param = fdnver_modRate;
        break;
    }
    case Param_hpf: {
        //param = mapValueOutsideRange(log(fdnver_highfreq), MIN_HPF_FREQUENCY_LOG, MAX_HPF_FREQUENCY_LOG);
        param = mapValueOutsideRange(fdnver_highfreq, MIN_HPF_FREQUENCY, MAX_HPF_FREQUENCY);
        break;
    }
    case Param_roomSize: {
        param = fdnver_roomSize;
        break;
    }
    case Param_decay: {
        param = fdnver_decay;
        break;
    }
    case Param_early: {
        param = t;
        break;
    }
    case Param_spread: {
        param = fdnver_spread;
        break;
    }
    case Param_freqDamp: {
        param = fdnver_freqDamp;
        break;
    }
    case Param_lpf: {
        param = mapValueOutsideRange(log(fdnver_lowfreq), MIN_LPF_FREQUENCY_LOG, MAX_LPF_FREQUENCY_LOG);
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
void Feedverb::getParameterLabel(VstInt32 index, char* label)
{
    switch (index) {
    case Param_mix: {
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    }
    case Param_modRate: {
        vst_strncpy(label, "Hz", kVstMaxParamStrLen);
        break;
    }
    case Param_modDepth: {
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    }
    case Param_roomSize: {
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    }
    case Param_hpf: {
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    }
    case Param_decay: {
        vst_strncpy(label, "s", kVstMaxParamStrLen);
        break;
    }
    case Param_early: {
        vst_strncpy(label, "ms", kVstMaxParamStrLen);
        break;
    }
    case Param_spread: {
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    }
    case Param_freqDamp: {
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    }
    case Param_lpf: {
        vst_strncpy(label, "Hz", kVstMaxParamStrLen);
        break;
    }
    default:
        break;
    }
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// return reverb parameters values for displaying purposes
void Feedverb::getParameterDisplay(VstInt32 index, char* text)
{
    switch (index) {
    case Param_mix: {
        float2string(fdnver_mix * 10, text, kVstMaxParamStrLen);
        break;
    }
    case Param_modRate: {      
        float2string(fdnver_modRate * MAX_MOD_RATE, text, kVstMaxParamStrLen);
        break;
    }
    case Param_modDepth: {       
        float2string(fdnver_modDepth, text, kVstMaxParamStrLen);
        break;
    }
    case Param_roomSize: {        
        float2string(fdnver_roomSize, text, kVstMaxParamStrLen);
        break;
    }
    case Param_hpf: {
        float2string(fdnver_highfreq, text, kVstMaxParamStrLen);
        break;
    }
    case Param_decay: {
        float2string(fdnver_decay * MAX_REVERB_DECAY_IN_SECONDS, text, kVstMaxParamStrLen);
        break;
    }
    case Param_early: {
        float2string(t, text, kVstMaxParamStrLen);
        break;
    }
    case Param_spread: {
        float2string(fdnver_spread, text, kVstMaxParamStrLen);
        break;
    }
    case Param_freqDamp: {
        float2string(fdnver_freqDamp, text, kVstMaxParamStrLen);
        break;
    }
    case Param_lpf: {
        float2string(fdnver_lowfreq, text, kVstMaxParamStrLen);
        break;
    }
    default:
        break;
    }
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// return the parameter's name for displaying purpose
void Feedverb::getParameterName(VstInt32 index, char* text)
{
    switch (index) {
    case Param_mix: {
        vst_strncpy(text, "Mix", kVstMaxParamStrLen);
        break;
    }
    case Param_modRate: {
        vst_strncpy(text, "Mod Rate", kVstMaxParamStrLen);
        break;
    }
    case Param_modDepth: {
        vst_strncpy(text, "Mod Depth", kVstMaxParamStrLen);
        break;
    }
    case Param_roomSize: {
        vst_strncpy(text, "Size", kVstMaxParamStrLen);
        break;
    }
    case Param_hpf: {
        vst_strncpy(text, "HPF", kVstMaxParamStrLen);
        break;
    }
    case Param_decay: {
        vst_strncpy(text, "Decay", kVstMaxParamStrLen);
        break;
    }
    case Param_early: {
        vst_strncpy(text, "Earl Len", kVstMaxParamStrLen);
        break;
    }
    case Param_spread: {
        vst_strncpy(text, "Spread", kVstMaxParamStrLen);
        break;
    }
    case Param_freqDamp: {
        vst_strncpy(text, "Dampening", kVstMaxParamStrLen);
        break;
    }
    case Param_lpf: {
        vst_strncpy(text, "LPF", kVstMaxParamStrLen);
        break;
    }
    default:
        break;
    }
}
/*--------------------------------------------------------------------*/


/* ------------------------------------------------------------------------------------------------------------
 -------------------------------------------  PROGRAM  --------------------------------------------------------
 ------------------------------------------------------------------------------------------------------------ */
//void Feedverb::setProgram(VstInt32 program)
//{
//    // Call the current implementation of "setProgram"
//    AudioEffect::setProgram(program);
//
//    // Create an instante of ReverbPresets with current preset
//    ReverbPresets* cp = &rev_presets[curProgram];
//
//    // Set each parameter
//    setParameter(Param_mix, cp->rev_wet);
//    setParameter(Param_decay, cp->rev_decay / MAX_REVERB_DECAY_IN_SECONDS);
//    setParameter(Param_damping, cp->rev_damping);
//    setParameter(Param_smearing, cp->rev_smearing);
//    setParameter(Param_spread, cp->rev_spread);
//    setParameter(Param_ModDepth, cp->rev_modDepth);
//    setParameter(Param_preDelay, cp->rev_preDelay / MAX_PREDELAY_VALUE_IN_MS);
//}
///*--------------------------------------------------------------------*/
//
///*--------------------------------------------------------------------*/
//// return program's current name
//void Feedverb::getProgramName(char* name)
//{
//    strcpy(name, rev_presets[curProgram].name);
//}
///*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
//// get the program's list
//bool Feedverb::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text)
//{
//    if (index < NUM_PRESETS)
//    {
//        strcpy(text, rev_presets[index].name);
//        return true;
//    }
//    return false;
//}
///*--------------------------------------------------------------------*/


/* ------------------------------------------------------------------------------------------------------------
 ---------------------------------------------  NAME  ---------------------------------------------------------
 ------------------------------------------------------------------------------------------------------------ */

 /*--------------------------------------------------------------------*/
bool Feedverb::getEffectName(char* name)
{
    vst_strncpy(name, "MisEfx", kVstMaxEffectNameLen);
    return true;
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
bool Feedverb::getVendorString(char* name)
{
    vst_strncpy(name, "Fox Suite", kVstMaxVendorStrLen);
    return true;
}
/*--------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------------------------------
 ------------------------------------------  DESTRUCTOR  ------------------------------------------------------
 ------------------------------------------------------------------------------------------------------------ */
Feedverb::~Feedverb() {
    delete fdnver_FDN;
}


