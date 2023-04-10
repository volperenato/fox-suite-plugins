//-------------------------------------------------------------------------------------------------------
//  MisEfx.h
//  Reverb plugin based on the Freeverb scheme
//  Customized by Renato Volpe on 07/02/2022.
//
//-------------------------------------------------------------------------------------------------------

#pragma once
#include <stdio.h>
#include "FDN.h"
#include "../vstsdk2.4/public.sdk/source/vst2.x/audioeffectx.h"
#include <math.h>


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
class Feedverb;

// Declare class to handle reverb's presets
class ReverbPresets {
	friend class Feedverb;
private:
	// Reverb User Parameters
	float rev_wet, rev_smearing, rev_decay, rev_damping, rev_lpfFreq, rev_hpfFreq, rev_preDelay, rev_modRate, rev_modDepth, rev_spread;
	char name[24];
};

//-------------------------------------------------------------------------------------------------------
class Feedverb : public AudioEffectX {

	
	// Initialize ReverbPresets instance
	ReverbPresets* rev_presets;

	// Reverb User Parameters
	float rev_wet, rev_smearing, rev_decay, rev_damping, rev_lpfFreq, rev_hpfFreq, rev_preDelay, rev_modRate, rev_modDepth, rev_spread;

	// FDN
	FDN* fdnverb_fdnLeft;
	FDN* fdnverb_fdnRight;

	
	void InitPlugin();
	void InitPresets();

public:

	Feedverb(audioMasterCallback audioMaster);
	~Feedverb();

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





