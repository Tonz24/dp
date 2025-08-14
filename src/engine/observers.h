//
// Created by Tonz on 11.08.2025.
//


#pragma once

#include "observer.h"

class KeyPressSubject : public Subject<int>{};
class KeyPressObserver : public Observer<int>{};