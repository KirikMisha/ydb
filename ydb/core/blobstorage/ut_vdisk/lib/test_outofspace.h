#pragma once 
 
#include "defs.h" 
#include "prepare.h" 
 
/////////////////////////////////////////////////////////////////////////// 
struct TWriteUntilOrangeZone { 
    void operator ()(TConfiguration *conf); 
}; 
 
/////////////////////////////////////////////////////////////////////////// 
struct TWriteUntilYellowZone { 
    void operator ()(TConfiguration *conf); 
}; 
 
