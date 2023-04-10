//-------------------------------------------------------------------------------------------------------
//  MisEfx.h
//  Reverb plugin based on the Freeverb scheme
//  Customized by Renato Volpe on 07/02/2022.
//
//-------------------------------------------------------------------------------------------------------

#pragma once
#include <stdio.h>
#include "AllPassFilter.h"
#include "Delay.h"
#include "LPCombFilter.h"
#include "ModDelay.h"
#include "HPFButterworth.h"
#include "Tremolo.h"
#include "Freeverb.h"
#include "../vstsdk2.4/public.sdk/source/vst2.x/audioeffectx.h"
#include <math.h>

#define NUM_COMB_FILTERS 8
#define NUM_ALLPASS_FILTERS_IN 3
#define NUM_ALLPASS_FILTERS_OUT 2
#define MAX_LPF_FREQUENCY 19000.0
#define MIN_LPF_FREQUENCY 20.0
#define MAX_HPF_FREQUENCY 17000.0
#define MIN_HPF_FREQUENCY 10.0

using namespace std;

// declare enum for reverb's parameters
enum EfxParameter {
	Param_wet = 0,
	Param_decay,
	Param_preDelay,
	Param_damping,
	Param_spread,
	Param_smearing,
	Param_lpfFreq,
	Param_hpfFreq,
	Param_ModRate,
	Param_ModDepth,
	Param_Count
};

// Declare class Reverb
class MisEfx;

// Declare class to handle reverb's presets
class ReverbPresets {
	friend class MisEfx;
private:
	// Reverb User Parameters
	float rev_wet, rev_smearing, rev_decay, rev_damping, rev_lpfFreq, rev_hpfFreq, rev_preDelay, rev_modRate, rev_modDepth, rev_spread;
	char name[24];
};

//-------------------------------------------------------------------------------------------------------
class MisEfx : public AudioEffectX {

	// const parameters
	const float MAX_LPF_FREQUENCY_LOG = log(MAX_LPF_FREQUENCY);
	const float MIN_LPF_FREQUENCY_LOG = log(MIN_LPF_FREQUENCY);
	const float MAX_HPF_FREQUENCY_LOG = log(MAX_HPF_FREQUENCY);
	const float MIN_HPF_FREQUENCY_LOG = log(MIN_HPF_FREQUENCY);

	// Initialize ReverbPresets instance
	ReverbPresets* rev_presets;

	// Reverb User Parameters
	float rev_wet, rev_smearing, rev_decay, rev_damping, rev_lpfFreq, rev_hpfFreq, rev_preDelay, rev_modRate, rev_modDepth, rev_spread;

	// Freeverb
	Freeverb* Reverb;

	// OscillatorType
	OscillatorType modWaveform;

	// LPF filter
	LPFButterworth* outputLPF;

	// HPF filter
	HPFButterworth* outputHPF;

	// Chorus
	//ModDelay* chorus;

	// Tremolo
	Tremolo* tremolo;

	void InitPlugin();
	float mapValueIntoRange(float value, float minvalue, float maxValue);
	float mapValueOutsideRange(float value, float minValue, float maxValue);
	void InitPresets();

public:


	MisEfx(audioMasterCallback audioMaster);
	~MisEfx();

	// Processing
	virtual void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames) override;
	virtual float getParameter(VstInt32 index) override;
	virtual void setParameter(VstInt32 index, float value) override;
	virtual bool getEffectName(char* name) override;
	virtual bool getVendorString(char* name) override;
	virtual void getParameterLabel(VstInt32 index, char* label) override;
	virtual void getParameterDisplay(VstInt32 index, char* text) override;
	virtual void getParameterName(VstInt32 index, char* text) override;
	virtual void setSampleRate(float sampleRate) override;
	virtual void setProgram(VstInt32 program) override;
	virtual void getProgramName(char* name) override;
	virtual bool getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text) override;
};





