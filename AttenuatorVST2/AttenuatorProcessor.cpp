//
//  AttenuatorProcessor.cpp
//  AttenuatorVST2
//
//  Created by Vlad Gorlov on 30.03.20.
//  Copyright © 2020 Vlad Gorlov. All rights reserved.
//

#include "AttenuatorProcessor.hpp"
#include "AttenuatorEditor.hpp" // 1️⃣ Imported new header.
#include <algorithm>

AudioEffect* createEffectInstance(audioMasterCallback audioMaster) {
   return new AttenuatorProcessor(audioMaster);
}

AttenuatorProcessor::AttenuatorProcessor(audioMasterCallback audioMaster)
: AudioEffectX(audioMaster, 1, 1) { // 1 program and 1 parameter only.
   setNumInputs (2);       // stereo in
   setNumOutputs (2);      // stereo out
   setUniqueID ('MyAg');   // identify. Kind of unique ID of plug-in.
   canProcessReplacing (); // supports replacing output
   canDoubleReplacing ();  // supports double precision processing
   
   mGain = 1.f;           // default to 0 dB
   mLastGain = mGain;
   vst_strncpy(programName, "Default", kVstMaxProgNameLen);   // default program name
   
   this->setEditor (new AttenuatorEditor(this)); // 2️⃣ Using editor.

   std::fill(mGainHistory.begin(), mGainHistory.end(), 0.0);
}

AttenuatorProcessor::~AttenuatorProcessor() {
   // Nothing to do at the moment.
}

// MARK: - Processing

void AttenuatorProcessor::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames) {

    if(sampleFrames == 0) { return; }

   float* in1  =  inputs[0];
   float* in2  =  inputs[1];
   float* out1 = outputs[0];
   float* out2 = outputs[1];

   for(VstInt32 i = 0; i < sampleFrames; ++i) {
      (*out1++) = (*in1++);
      (*out2++) = (*in2++);
   }

   float newGain = getSmoothedValue(mGain);
   float gainStep = (newGain - mLastGain) / sampleFrames;

   for(VstInt32 i = 0; i < sampleFrames; ++i) {
      (*out1++) = (*in1++) * (mLastGain + gainStep * i);
      (*out2++) = (*in2++) * (mLastGain + gainStep * i);
   }

   mLastGain = newGain;
}

void AttenuatorProcessor::processDoubleReplacing (double** inputs, double** outputs, VstInt32 sampleFrames) {

   if(sampleFrames == 0) { return; }

   double* in1  =  inputs[0];
   double* in2  =  inputs[1];
   double* out1 = outputs[0];
   double* out2 = outputs[1];


   for(VstInt32 i = 0; i < sampleFrames; ++i) {
      (*out1++) = (*in1++);
      (*out2++) = (*in2++);
   }

   float newGain = getSmoothedValue(mGain);
   float gainStep = (newGain - mLastGain) / sampleFrames;

   for(VstInt32 i = 0; i < sampleFrames; ++i) {
      (*out1++) = (*in1++) * (mLastGain + gainStep * i);
      (*out2++) = (*in2++) * (mLastGain + gainStep * i);
   }

   mLastGain = newGain;
}

// MARK: - Programs

void AttenuatorProcessor::setProgramName (char* name) {
   vst_strncpy(programName, name, kVstMaxProgNameLen);
}

void AttenuatorProcessor::getProgramName (char* name) {
   vst_strncpy(name, programName, kVstMaxProgNameLen);
}

// MARK: - Parameters

void AttenuatorProcessor::setParameter (VstInt32 index, float value) {
   mGain = value;
   bool shouldUpdate = editor && editor->isOpen() && !mIsUpdatingGain;
   if(shouldUpdate) {
      ((AttenuatorEditor *)editor)->setParameter(index, value); // 4️⃣ Updating UI from VST Host.
   }
}

void AttenuatorProcessor::setParameterAutomated (VstInt32 index, float value) {
   mIsUpdatingGain = true; // 3️⃣ Needed to avoid loopback.
   AudioEffectX::setParameterAutomated(index, value);
   mIsUpdatingGain = false; // 3️⃣ Needed to avoid loopback.
}

float AttenuatorProcessor::getParameter (VstInt32 index) {
   return mGain;
}

void AttenuatorProcessor::getParameterName (VstInt32 index, char* label) {
   vst_strncpy(label, "Gain", kVstMaxParamStrLen);
}

void AttenuatorProcessor::getParameterDisplay (VstInt32 index, char* text) {
   dB2string(mGain, text, kVstMaxParamStrLen);
}

void AttenuatorProcessor::getParameterLabel (VstInt32 index, char* label) {
   vst_strncpy(label, "dB", kVstMaxParamStrLen);
}

// MARK: - Metadata

bool AttenuatorProcessor::getEffectName (char* name) {
   vst_strncpy(name, "Attenuator VST2", kVstMaxEffectNameLen);
   return true;
}

bool AttenuatorProcessor::getProductString (char* text) {
   vst_strncpy(text, "Examples", kVstMaxProductStrLen);
   return true;
}

bool AttenuatorProcessor::getVendorString (char* text) {
   vst_strncpy(text, "Vlad Gorlov", kVstMaxVendorStrLen);
   return true;
}

float AttenuatorProcessor::getSmoothedValue(float newGain)
{
    mGainHistory[mGainHistoryIndex] = newGain;
    float ret = 0;
    for(int i = 0; i < mGainHistory.size(); ++i) {
        ret += mGainHistory[i];
    }
    ret /= mGainHistory.size();
    mGainHistoryIndex += 1;
    if(mGainHistoryIndex >= mGainHistory.size()) { mGainHistoryIndex = 0; }

    return ret;
}
