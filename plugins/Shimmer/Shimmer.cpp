//-------------------------------------------------------------------------------------------------------
//  Shimmer.cpp
//  BranchReverb plugin based on the Freeverb scheme
//  Customized by Renato Volpe on 07/02/2022.
//
//-------------------------------------------------------------------------------------------------------

#include "Shimmer.h"
#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <math.h>
#include "utils.h"

/*--------------------------------------------------------------------*/
// Plugin constants
#define NUM_PRESETS 5
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// FDN constants
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
#define BRANCH_REVERB_DECAY 6.0
const float MIN_DAMPING_FREQUENCY_LOG = log(MIN_DAMPING_FREQUENCY);
const float MAX_DAMPING_FREQUENCY_LOG = log(MAX_DAMPING_FREQUENCY);
const float LPF_FILTER_MAX_FREQ_LOG = log(LPF_FILTER_MAX_FREQ);
const float LPF_FILTER_MIN_FREQ_LOG = log(LPF_FILTER_MIN_FREQ);
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// Pitch shifter constants
#define NUM_OF_PITCH_INTERVALS_ALLOWED 10
const float DELTA_PARAMETER_BETWEEN_INTERVALS = 1.0/NUM_OF_PITCH_INTERVALS_ALLOWED;
char* INTERVALS_NAMES_STRING[NUM_OF_PITCH_INTERVALS_ALLOWED] = { "2nd Maj", "3rd Min", "3rd Maj", "4th Per", "5th Per", "6th Maj", "7th Maj", "1st Oct", "1 Oct+5", "1+2 Oct"};
const float INTERVALS_IN_SEMITONES_PITCH1[NUM_OF_PITCH_INTERVALS_ALLOWED] = {2.0, 3.0, 4.0, 5.0, 7.0, 9.0, 11.0, 12.0, 12.0, 12.0};
const float INTERVALS_IN_SEMITONES_PITCH2[NUM_OF_PITCH_INTERVALS_ALLOWED] = {  0,   0,   0,   0,   0,   0,    0,    0,   19,   24};
/*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*/
// Mix dry and wet signals with equal power distribution
void Shimmer::updateMix() {
    _wet = sin(shim_mix * M_PI * 0.5);
    _dry = cos(shim_mix * M_PI * 0.5);
}

// If two pitch shifters are giving output, sum them with half gain each
void Shimmer::updateMixPitchShifters(float pitch2) {
    _mixP1 = 1.0;
    _mixP2 = 0.0;
    if (pitch2 != 0.0) {
        _mixP1 = 0.5;
        _mixP2 = 0.5;
    }
}

/*--------------------------------------------------------------------*/
// Shimmer class constructor
Shimmer::Shimmer(audioMasterCallback audioMaster)
    : AudioEffectX(audioMaster, NUM_PRESETS, Param_Count) // n program, n parameters
{
    setNumInputs(2);		// stereo in
    setNumOutputs(2);		// stereo out
    setUniqueID('Fox');	    // identify    
    InitPlugin();
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
    return new Shimmer(audioMaster);
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// Initialize all the objects and parameters
void Shimmer::InitPlugin()
{
    /*.......................................*/
    // initialize reverb plug-in parameters
    InitPresets();

    // get current sample rate
    int sampleRate = getSampleRate();
    shim_mix = 0.5;    
    shim_roomSize = 0.5;
    shim_decay = 0.2;
    shim_damping = 0.5;    
    shim_spread = 0.5;
    shim_shimmer = 0.5;
    shim_intervals = 0.1;
    shim_modRate = 0.0;
    shim_modDepth = 0.0;
    shim_lpf = MAX_LPF_FREQUENCY;
    shim_hpf = MIN_HPF_FREQUENCY;           
    updateMix();

    /*.......................................*/
    // Create FDN Branch Reverb
    BranchReverb = new FDN(2, DEFAULT_NUMBER_OF_INTERNAL_CHANNELS_FDN, 2, NUMBER_OF_DIFFUSION_STEPS, 1);

    // Initialize objects (allocate delay lines)
    BranchReverb->initialize(DIFFUSER_DELAY_BUFFER_SIZE_MS, FEEDBACK_DELAY_BUFFER_SIZE_MS, sampleRate);

    // Set room size
    BranchReverb->setRoomSize(shim_roomSize);

    // Set decay
    BranchReverb->setDecayInSeconds(0.25 * shim_decay * MAX_REVERB_DECAY_IN_SECONDS);

    // set damping frequency
    BranchReverb->setDampingFrequency(exp(mapValueIntoRange(1.0 - shim_damping, MIN_DAMPING_FREQUENCY_LOG, MAX_DAMPING_FREQUENCY_LOG)));
    BranchReverb->setDampingType(DAMPING_LPF_TYPE);

    // set output low & high pass filters
    BranchReverb->setLowPassFrequency(LPF_FILTER_MAX_FREQ);
    BranchReverb->setLowPassType(OUTPUT_LPF_TYPE);
    BranchReverb->setHighPassType(OUTPUT_HPF_TYPE);
    BranchReverb->setHighPassFrequency(HPF_FILTER_MIN_FREQ);

    // Modulation
    BranchReverb->setModDepth(0.0);
    BranchReverb->setModRate(0.0);

    // stereo spread
    BranchReverb->setStereoSpread(0.5);

    // output mixing mode
    BranchReverb->setMixMode(MixMode::First);
    /*.......................................*/

    /*.......................................*/
    // Create FDN Master Reverb
    MasterReverb = new FDN(2, DEFAULT_NUMBER_OF_INTERNAL_CHANNELS_FDN, 2, NUMBER_OF_DIFFUSION_STEPS, 1);

    // Initialize objects (allocate delay lines)
    MasterReverb->initialize(DIFFUSER_DELAY_BUFFER_SIZE_MS, FEEDBACK_DELAY_BUFFER_SIZE_MS, sampleRate);

    // Set room size
    MasterReverb->setRoomSize(shim_roomSize);

    // Set decay
    MasterReverb->setDecayInSeconds(shim_decay * MAX_REVERB_DECAY_IN_SECONDS);

    // set damping frequency
    MasterReverb->setDampingFrequency(exp(mapValueIntoRange(1.0 - shim_damping, MIN_DAMPING_FREQUENCY_LOG, MAX_DAMPING_FREQUENCY_LOG)));
    MasterReverb->setDampingType(DAMPING_LPF_TYPE);

    // set output low & high pass filters
    MasterReverb->setLowPassFrequency(shim_lpf);
    MasterReverb->setLowPassType(OUTPUT_LPF_TYPE);
    MasterReverb->setHighPassType(OUTPUT_HPF_TYPE);
    MasterReverb->setHighPassFrequency(shim_hpf);

    // Modulation
    MasterReverb->setModDepth(shim_modDepth);
    MasterReverb->setModRate(shim_modRate);

    // stereo spread
    MasterReverb->setStereoSpread(shim_spread);

    // output mixing mode
    MasterReverb->setMixMode(MixMode::First);
    /*.......................................*/
 
    /*.......................................*/
    // init PSMVocoder
    PitchShift_1octL = new PSMVocoder();
    PitchShift_1octR = new PSMVocoder();
    PitchShift_2octL = new PSMVocoder();
    PitchShift_2octR = new PSMVocoder();
    // set sample rate
    PitchShift_1octL->reset((double)sampleRate);
    PitchShift_1octR->reset((double)sampleRate);
    PitchShift_2octL->reset((double)sampleRate);
    PitchShift_2octR->reset((double)sampleRate);
    // set phase locking and peak tracking
    PSMVocoderParameters params1L = PitchShift_1octL->getParameters();
    PSMVocoderParameters params1R = PitchShift_1octR->getParameters();
    PSMVocoderParameters params2L = PitchShift_2octL->getParameters();
    PSMVocoderParameters params2R = PitchShift_2octR->getParameters();    
    params1L.enablePeakPhaseLocking = true;
    params1R.enablePeakPhaseLocking = true;
    params2L.enablePeakPhaseLocking = true;
    params2R.enablePeakPhaseLocking = true;
    params1L.enablePeakTracking = true;
    params1R.enablePeakTracking = true;
    params2L.enablePeakTracking = true;
    params2R.enablePeakTracking = true;

    PitchShift_1octL->setParameters(params1L);
    PitchShift_1octR->setParameters(params1R);
    PitchShift_2octL->setParameters(params2L);
    PitchShift_2octR->setParameters(params2R);

    PitchShift_1octL->setPitchShift(12.0);
    PitchShift_1octR->setPitchShift(12.0);
    PitchShift_2octL->setPitchShift(24.0);
    PitchShift_2octR->setPitchShift(24.0);
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// replace the "setSampleRate" method with user-defined one
void Shimmer::setSampleRate(float sampleRate)
{
    // Call AudioEffect "setSampleRate" method
    AudioEffect::setSampleRate(sampleRate);

    // Call setSampleRate on every needed module
    BranchReverb->setSampleRate(sampleRate);
    MasterReverb->setSampleRate(sampleRate);
    PitchShift_1octL->reset(sampleRate);
    PitchShift_1octR->reset(sampleRate);
    PitchShift_2octL->reset(sampleRate);
    PitchShift_2octR->reset(sampleRate);
}
/*--------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------------------------------
  -------------------------------------  PROCESS REPLACING  ---------------------------------------------------
  ------------------------------------------------------------------------------------------------------------ */
void Shimmer::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
    // Extract input and output buffers
    float* inL = inputs[0]; // buffer input left
    float* inR = inputs[1]; // buffer input right

    float* outL = outputs[0]; // buffer output left
    float* outR = outputs[1]; // buffer output right

    // write input to file
    //string pre = "test_input.txt";
    //WriteBufferToFile(inputs, sampleFrames, pre);
    
    // Cycle over the sample frames number
    for (int i = 0; i < sampleFrames; i++) {

        // Create tmp arrays for processing
        float pitch_input[2] = {inL[i], inR[i]};
        float pitch_output_1oct[2] = { 0.0, 0.0 };
        float pitch_output_2oct[2] = { 0.0, 0.0 };        
        float pitch_summed_output[2] = { pitch_input[0], pitch_input[1] };
        float bran_rev_out[2] = { 0.0, 0.0 };
        float mast_rev_out[2] = { 0.0, 0.0 };
        float mast_rev_in[2];

        // --- Pitch Shifting        
        // Process pitch shifting 1 octave
        pitch_output_1oct[0] = PitchShift_1octL->processAudioSample(pitch_input[0]);
        pitch_output_1oct[1] = PitchShift_1octR->processAudioSample(pitch_input[1]);

        // Process pitch shifting 2 octaves
        pitch_output_2oct[0] = PitchShift_2octL->processAudioSample(pitch_input[0]);
        pitch_output_2oct[1] = PitchShift_2octR->processAudioSample(pitch_input[1]);

        // Sum outputs
        pitch_summed_output[0] = _mixP1 * pitch_output_1oct[0] + _mixP2 * pitch_output_2oct[0];
        pitch_summed_output[1] = _mixP1 * pitch_output_1oct[1] + _mixP2 * pitch_output_2oct[1];
              
        // --- Branch Reverb        
        BranchReverb->processAudio(pitch_summed_output, bran_rev_out);

        // --- Master Reverb        
        // Mix branch reverb output with dry input
        mast_rev_in[0] = shim_shimmer * bran_rev_out[0] + (1 - shim_shimmer) * inL[i];
        mast_rev_in[1] = shim_shimmer * bran_rev_out[1] + (1 - shim_shimmer) * inR[i];

        // Process master reverb        
        MasterReverb->processAudio(mast_rev_in, mast_rev_out);        

        // Stereo spread processing + output allocation
        outL[i] = _wet * mast_rev_out[0] + _dry * inL[i];
        outR[i] = _wet * mast_rev_out[1] + _dry * inR[i];
    }

   // Write samples to file
   //string post = "test_output.txt";
   //WriteBufferToFile(outputs, sampleFrames, post);

}
/*--------------------------------------------------------------------*/



/* ------------------------------------------------------------------------------------------------------------
  ------------------------------------------  PARAMETERS  ------------------------------------------------------
  ------------------------------------------------------------------------------------------------------------ */
  // set reverb parameters values
void Shimmer::setParameter(VstInt32 index, float value)
{
    switch (index) {
    case Param_mix: {
        shim_mix = value;        
        updateMix();
        break;
    }
    case Param_roomSize: {
        shim_roomSize = value;
        BranchReverb->setRoomSize(shim_roomSize);
        MasterReverb->setRoomSize(shim_roomSize);
        break;
    }
    case Param_shimmer: {
        shim_shimmer = value;
        break;
    }
    case Param_decay: {
        shim_decay = value;
        BranchReverb->setDecayInSeconds(0.25 * shim_decay * MAX_REVERB_DECAY_IN_SECONDS);
        MasterReverb->setDecayInSeconds(shim_decay * MAX_REVERB_DECAY_IN_SECONDS);
        break;
    }    
    case Param_damping: {
        shim_damping = value; 
        float freq = exp(mapValueIntoRange(1.0 - shim_damping, MIN_DAMPING_FREQUENCY_LOG, MAX_DAMPING_FREQUENCY_LOG));
        BranchReverb->setDampingFrequency(freq);
        MasterReverb->setDampingFrequency(freq);
        break;
    }        
    case Param_spread: {
        shim_spread = value;
        MasterReverb->setStereoSpread(shim_spread);
        break;
    }
    case Param_shimIntrvals: {        
        if (value == 1)
            value = 0.99; // if value = 1, then pitIdx = NUM_OF_PITCH_INTRVL_ALLOWED + 1 -> outside of array boundaries
        shim_intervals = value;
        int pitIdx = shim_intervals / DELTA_PARAMETER_BETWEEN_INTERVALS;
        PitchShift_1octL->setPitchShift(INTERVALS_IN_SEMITONES_PITCH1[pitIdx]);
        PitchShift_1octR->setPitchShift(INTERVALS_IN_SEMITONES_PITCH1[pitIdx]);
        PitchShift_2octL->setPitchShift(INTERVALS_IN_SEMITONES_PITCH2[pitIdx]);
        PitchShift_2octR->setPitchShift(INTERVALS_IN_SEMITONES_PITCH2[pitIdx]);
        updateMixPitchShifters(INTERVALS_IN_SEMITONES_PITCH2[pitIdx]);
        break;
    }
    case Param_modDepth: {
        shim_modDepth = value;
        MasterReverb->setModDepth(shim_modDepth);
        break;
    }
    case Param_modRate: {
        shim_modRate = value;
        MasterReverb->setModRate(shim_modRate * MAX_MOD_RATE);
        break;
    }
    case Param_lpf: {
        shim_lpf = exp(mapValueIntoRange(value, LPF_FILTER_MIN_FREQ_LOG, LPF_FILTER_MAX_FREQ_LOG));
        MasterReverb->setLowPassFrequency(shim_lpf);
        break;
    }
    case Param_hpf: {        
        shim_hpf = mapValueIntoRange(value, HPF_FILTER_MIN_FREQ, HPF_FILTER_MAX_FREQ);
        MasterReverb->setHighPassFrequency(shim_hpf);
        break;
    }    
    default:
        break;
    }
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// return reverb parameters values
float Shimmer::getParameter(VstInt32 index)
{
    float param = 0;
    switch (index) {
    case Param_mix:
    {
        param = shim_mix;
        break;
    }
    case Param_roomSize: {
        param = shim_roomSize;
        break;
    }
    case Param_shimmer:
    {
        param = shim_shimmer;
        break;
    }
    case Param_decay:
    {
        param = shim_decay;
        break;
    }    
    case Param_damping:
    {
        param = shim_damping;
        break;
    }    
    case Param_spread:
    {
        param = shim_spread;
        break;
    }
    case Param_shimIntrvals:
    {
        param = shim_intervals;
        break;
    }
    case Param_modDepth: {
        param = shim_modDepth;
        break;
    }
    case Param_modRate: {
        param = shim_modRate;
        break;
    }
    case Param_lpf: {
        param = mapValueOutsideRange(log(shim_lpf), MIN_LPF_FREQUENCY_LOG, MAX_LPF_FREQUENCY_LOG);
        break;
    }
    case Param_hpf: {        
        param = mapValueOutsideRange(shim_hpf, HPF_FILTER_MIN_FREQ, HPF_FILTER_MAX_FREQ);
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
void Shimmer::getParameterLabel(VstInt32 index, char* label)
{
    switch (index) {
    case Param_mix: {
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    }
    case Param_roomSize: {
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    }
    case Param_shimmer: {
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    }
    case Param_decay: {
        vst_strncpy(label, "s", kVstMaxParamStrLen);
        break;
    }
    case Param_damping: {
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    }
    case Param_spread: {
        vst_strncpy(label, "", kVstMaxParamStrLen);
        break;
    }
    case Param_shimIntrvals: {
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
    case Param_lpf: {
        vst_strncpy(label, "Hz", kVstMaxParamStrLen);
        break;
    }
    case Param_hpf: {
        vst_strncpy(label, "Hz", kVstMaxParamStrLen);
        break;
    }      
    default: {
        break;
    }
    }
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// return reverb parameters values for displaying purposes
void Shimmer::getParameterDisplay(VstInt32 index, char* text)
{    
    switch (index) {
    case Param_mix: {
        float2string(shim_mix * 10, text, kVstMaxParamStrLen);
        break;
    }
    case Param_roomSize: {
        float2string(shim_roomSize * 10, text, kVstMaxParamStrLen);
        break;
    }
    case Param_shimmer: {
        float2string(shim_shimmer * 10, text, kVstMaxParamStrLen);
        break;
    }
    case Param_decay: {
        float2string(shim_decay * MAX_REVERB_DECAY_IN_SECONDS, text, kVstMaxParamStrLen);
        break;
    }
    case Param_damping: {
        float2string(shim_damping * 10, text, kVstMaxParamStrLen);
        break;
    }
    case Param_spread: {
        float2string(shim_spread * 10, text, kVstMaxParamStrLen);
        break;
    }
    case Param_shimIntrvals: {
        int pitIdx = shim_intervals / DELTA_PARAMETER_BETWEEN_INTERVALS;
        vst_strncpy(text, INTERVALS_NAMES_STRING[pitIdx], kVstMaxParamStrLen);
        break; 
    }
    case Param_modRate: {
        float2string(shim_modRate * MAX_MOD_RATE, text, kVstMaxParamStrLen);
        break;
    }
    case Param_modDepth: {
        float2string(shim_modDepth * 10, text, kVstMaxParamStrLen);
        break;
    }
    case Param_lpf: {
        float2string(shim_lpf, text, kVstMaxParamStrLen);
        break;
    }
    case Param_hpf: {
        float2string(shim_hpf, text, kVstMaxParamStrLen);
        break;
    }      
    default: {
        break;
    }
    }
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// return the parameter's name for displaying purpose
void Shimmer::getParameterName(VstInt32 index, char* text)
{
    switch (index) {
    case Param_mix: {
        vst_strncpy(text, "Wet", kVstMaxParamStrLen);
        break;
    }
    case Param_roomSize: {
        vst_strncpy(text, "Size", kVstMaxParamStrLen);
        break;
    }
    case Param_shimmer: {
        vst_strncpy(text, "Shimmer", kVstMaxParamStrLen);
        break;
    }
    case Param_decay: {
        vst_strncpy(text, "Decay", kVstMaxParamStrLen);
        break;
    }
    case Param_damping: {
        vst_strncpy(text, "Damping", kVstMaxParamStrLen);
        break;
    }
    case Param_spread: {
        vst_strncpy(text, "Stereo", kVstMaxParamStrLen);
        break;
    }
    case Param_shimIntrvals: {
        vst_strncpy(text, "Intervals", kVstMaxParamStrLen);
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
    case Param_lpf: {
        vst_strncpy(text, "LPF", kVstMaxParamStrLen);
        break;
    }
    case Param_hpf: {
        vst_strncpy(text, "HPF", kVstMaxParamStrLen);
        break;
    }     
    default: {
        break;
    }
    }
}
/*--------------------------------------------------------------------*/


/* ------------------------------------------------------------------------------------------------------------
 -------------------------------------------  PROGRAM  --------------------------------------------------------
 ------------------------------------------------------------------------------------------------------------ */

 /*--------------------------------------------------------------------*/
 // Define presets parameters values
void Shimmer::InitPresets()
{
    shim_presets = new ShimmerPresets[NUM_PRESETS];

    /*----------------------------------------------------*/
    // "Default" preset
    strcpy(shim_presets[0].name, "Default");
    shim_presets[0].shim_mix = 0.2;                        // given as a number between 0 and 1
    shim_presets[0].shim_decay = 1.0;                      // given in seconds
    shim_presets[0].shim_shimmer = 0.7;                   // given as a number between 0 and 1
    shim_presets[0].shim_damping = 0.5;                    // given as a number between 0 and 1
    shim_presets[0].shim_spread = 0.3;                     // given as a number between 0 and 1

    /*----------------------------------------------------*/
    // "Dreamy" preset
    strcpy(shim_presets[1].name, "Dreamy");
    shim_presets[1].shim_mix = 0.5;
    shim_presets[1].shim_decay = 3.3;
    shim_presets[1].shim_shimmer = 0.8;
    shim_presets[1].shim_damping = 0.6;
    shim_presets[1].shim_spread = 1.0;

    /*----------------------------------------------------*/
    // "Short" preset
    strcpy(shim_presets[2].name, "Short");
    shim_presets[2].shim_mix = 0.2;
    shim_presets[2].shim_decay = 2;
    shim_presets[2].shim_shimmer = 0.5;
    shim_presets[2].shim_damping = 0.4;
    shim_presets[2].shim_spread = 0.2;

    /*----------------------------------------------------*/
    // "Metallic" preset
    strcpy(shim_presets[3].name, "Metallic");
    shim_presets[3].shim_mix = 0.5;
    shim_presets[3].shim_decay = 2.2;
    shim_presets[3].shim_shimmer = 0.0;
    shim_presets[3].shim_damping = 0.0;
    shim_presets[3].shim_spread = 1.0;

    /*----------------------------------------------------*/
    // "Wobbly" preset
    strcpy(shim_presets[4].name, "Wobbly");
    shim_presets[4].shim_mix = 0.65;
    shim_presets[4].shim_decay = 2.0;
    shim_presets[4].shim_shimmer = 0.7;
    shim_presets[4].shim_damping = 0.3;
    shim_presets[4].shim_spread = 0.3;

    // Set the program when creating a new plugin instance
    int initIdx = 0;
    AudioEffect::setProgram(initIdx);
    shim_mix = shim_presets[initIdx].shim_mix;
    shim_decay = shim_presets[initIdx].shim_decay;
    shim_shimmer = shim_presets[initIdx].shim_shimmer;
    shim_damping = shim_presets[initIdx].shim_damping;
    shim_spread = shim_presets[initIdx].shim_spread;
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
void Shimmer::setProgram(VstInt32 program)
{
    // Call the current implementation of "setProgram"
    AudioEffect::setProgram(program);

    // Create an instante of ShimmerPresets with current preset
    ShimmerPresets* cp = &shim_presets[curProgram];

    // Set each parameter
    setParameter(Param_mix, cp->shim_mix);
    setParameter(Param_decay, cp->shim_decay / MAX_REVERB_DECAY_IN_SECONDS);
    setParameter(Param_damping, cp->shim_damping);
    setParameter(Param_shimmer, cp->shim_shimmer);
    setParameter(Param_spread, cp->shim_spread);
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// return program's current name
void Shimmer::getProgramName(char* name)
{
    strcpy(name, shim_presets[curProgram].name);
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
// get the program's list
bool Shimmer::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text)
{
    if (index < NUM_PRESETS)
    {
        strcpy(text, shim_presets[index].name);
        return true;
    }
    return false;
}
/*--------------------------------------------------------------------*/


/* ------------------------------------------------------------------------------------------------------------
 ---------------------------------------------  NAME  ---------------------------------------------------------
 ------------------------------------------------------------------------------------------------------------ */

 /*--------------------------------------------------------------------*/
bool Shimmer::getEffectName(char* name)
{
    vst_strncpy(name, "Shimmer", kVstMaxEffectNameLen);
    return true;
}
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
bool Shimmer::getVendorString(char* name)
{
    vst_strncpy(name, "Fox Suite", kVstMaxVendorStrLen);
    return true;
}
/*--------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------------------------------
 ------------------------------------------  DESTRUCTOR  ------------------------------------------------------
 ------------------------------------------------------------------------------------------------------------ */
Shimmer::~Shimmer()
{
    //Free BranchReverb, delay and pitch shifters
    delete MasterReverb;
    delete BranchReverb;
    delete PitchShift_1octL, PitchShift_1octR, PitchShift_2octL, PitchShift_2octR;
}


