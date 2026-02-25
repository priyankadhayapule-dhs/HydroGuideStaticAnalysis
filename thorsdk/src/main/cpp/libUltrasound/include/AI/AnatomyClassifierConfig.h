//
// Copyright 2019 EchoNous Inc.
//
#pragma once

// Configuration constants for the CardiacAnnotator model
// to be consistent with other models  -  actual size: 7
struct AnatomyClassifierPred
{
    float organ[8];
};

const char* const ANATOMY_CLASSES[] =
{
    "CARDIAC", "ABDOMINAL", "NOISE", "NOT SURE", "LUNG", "SUBCOSTAL", "IVC"
};

enum AnatomyClass
{
    CARDIAC, ABDOMINAL, NOISE, NOT_SURE, LUNG, SUBCOSTAL, IVC
};


// TODO: UPDATE - Phased Array ID
enum OrganId
{
    HEART, LUNGS, ABDOMEN
};