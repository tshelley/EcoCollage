/*
 * Symbol.cpp
 *
 *  Created on: Jun 25, 2013
 *      Author: brianna
 */

#include "Symbol.h"
#include "opencv/cv.h"

namespace surf {


Symbol::Symbol(IplImage* img, int x, int y) {
	// TODO Auto-generated constructor stub
	this->x = x;
	this->y = y;
	this->img = img;

}

Symbol::~Symbol() {
	// TODO Auto-generated destructor stub
}


} /* namespace surf */
