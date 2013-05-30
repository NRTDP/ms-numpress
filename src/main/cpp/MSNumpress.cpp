//
// MSNumpress.cpp
// johan.teleman@immun.lth.se
//

#include <stdio.h>
#include <stdint.h>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <cmath>
#include "MSNumpress.hpp"

namespace ms {
namespace numpress {

using std::cout;
using std::cerr;
using std::endl;
using std::min;


/**
 * Encodes the int x as a number of halfbytes in res. 
 * res_length is incremented by the number of halfbytes, 
 * which will be 1 <= n <= 9
 */
void encodeInt(
		int x,
		unsigned char* res,
		size_t *res_length	
) {
	int i, l, m;
	int mask = 0xf0000000;
	int init = x & mask;

	if (init == 0) {
		l = 8;
		for (i=0; i<8; i++) {
			m = mask >> (4*i);
			if ((x & m) != 0) {
				l = i;
				break;
			}
		}
		res[0] = l;
		for (i=l; i<8; i++) {
			res[1+i-l] = x >> (4*(i-l));
		}
		*res_length += 1+8-l;

	} else if (init == mask) {
		l = 7;
		for (i=0; i<8; i++) {
			m = mask >> (4*i);
			if ((x & m) != m) {
				l = i;
				break;
			}
		}
		res[0] = l + 8;
		for (i=l; i<8; i++) {
			res[1+i-l] = x >> (4*(i-l));
		}
		*res_length += 1+8-l;

	} else {
		res[0] = 0;
		for (i=0; i<8; i++) {
			res[1+i] = x >> (4*i);
		}
		*res_length += 9;

	}
}

void MSNumpress::encodeLinear(
		const double *data, 
		size_t dataSize, 
		unsigned char *result, 
		size_t *resultByteCount
) {
	unsigned int ints[3];
	size_t i, ri;
	unsigned char halfBytes[10];
	size_t halfByteCount;
	size_t hbi;
	int extrapol;
	int diff;

	//printf("Encoding %d doubles\n", (int)dataSize);

	ints[1] = data[0] * 100000 + 0.5;
	ints[2] = data[1] * 100000 + 0.5;
	
	for (i=0; i<4; i++) {
		result[0+i] = (ints[1] >> (i*8)) & 0xff;
		result[4+i] = (ints[2] >> (i*8)) & 0xff;
	}

	halfByteCount = 0;
	ri = 8;

	for (i=2; i<dataSize; i++) {
		ints[0] = ints[1];
		ints[1] = ints[2];
		ints[2] = data[i] * 100000 + 0.5;
		extrapol = ints[1] + (ints[1] - ints[0]);
		diff = ints[2] - extrapol;
		//printf("%d %d %d,   extrapol: %d    diff: %d \n", ints[0], ints[1], ints[2], extrapol, diff);
		encodeInt(diff, &halfBytes[halfByteCount], &halfByteCount);
		/*
		printf("%d (%d):  ", diff, (int)halfByteCount);
		for (j=0; j<halfByteCount; j++) {
			printf("%x ", halfBytes[j] & 0xf);
		}
		printf("\n");
		*/
		
		
		for (hbi=1; hbi < halfByteCount; hbi+=2) {
			result[ri] = (halfBytes[hbi-1] << 4) | (halfBytes[hbi] & 0xf);
			//printf("%x \n", result[ri]);
			ri++;
		}
		if (halfByteCount % 2 != 0) {
			halfBytes[0] = halfBytes[halfByteCount-1];
			halfByteCount = 1;
		} else {
			halfByteCount = 0;
		}
	}
	if (halfByteCount == 1) {
		result[ri] = halfBytes[0] << 4;
		ri++;
	}
	*resultByteCount = ri;
}



void MSNumpress::encodeCount(
		const double *data, 
		size_t dataSize, 
		unsigned char *result, 
		size_t *resultByteCount
) {
	size_t i, ri, count, j;
	unsigned char halfBytes[10];
	size_t halfByteCount;
	size_t hbi;

	//printf("Encoding %d doubles\n", (int)dataSize);

	halfByteCount = 0;
	ri = 0;

	for (i=0; i<dataSize; i++) {
		count = data[i] + 0.5;
		//printf("%d %d %d,   extrapol: %d    diff: %d \n", ints[0], ints[1], ints[2], extrapol, diff);
		encodeInt(count, &halfBytes[halfByteCount], &halfByteCount);
		/*
		printf("%d (%d):  ", count, (int)halfByteCount);
		for (j=0; j<halfByteCount; j++) {
			printf("%x ", halfBytes[j] & 0xf);
		}
		printf("\n");
		*/
		
		for (hbi=1; hbi < halfByteCount; hbi+=2) {
			result[ri] = (halfBytes[hbi-1] << 4) | (halfBytes[hbi] & 0xf);
			//printf("%x \n", result[ri]);
			ri++;
		}
		if (halfByteCount % 2 != 0) {
			halfBytes[0] = halfBytes[halfByteCount-1];
			halfByteCount = 1;
		} else {
			halfByteCount = 0;
		}
	}
	if (halfByteCount == 1) {
		result[ri] = halfBytes[0] << 4;
		ri++;
	}
	*resultByteCount = ri;
}


/**
 * Decodes an int from the half bytes in bp. Lossless reverse of encode_int 
 */
void decodeInt(
		const std::vector<unsigned char> &data,
		size_t *di,
		int *half,
		int *res
) {
	size_t n;
	size_t i;
	int mask, m;
	unsigned char head;
	unsigned char hb;

	if (*half == 0) {
		head = data[*di] >> 4;
	} else {
		head = data[*di] & 0xf;
		(*di)++;
	}

	*half = 1-(*half);
	*res = 0;
	
	if (head <= 8) {
		n = head;
	} else { // leading ones, fill n half bytes in res
		n = head - 8;
		mask = 0xf0000000;
		for (i=0; i<n; i++) {
			m = mask >> (4*i);
			*res = *res | m;
		}
	}
	
	if (n == 8) {
		return;
	}
	
	for (i=n; i<8; i++) {
		if (*half == 0) {
			hb = data[*di] >> 4;
		} else {
			hb = data[*di] & 0xf;
			(*di)++;
		}
		*res = *res | (hb << ((i-n)*4));
		*half = 1 - (*half);
	}
}

void MSNumpress::decodeLinear(
		std::vector<unsigned char> &data,  
		std::vector<double> &result
) {
	size_t i;
	size_t ri;
	int init;
	int ints[3];
	//double d;
	size_t di;
	size_t dataSize = data.size();
	int half;
	int extrapol;
	int y;

	try {
		ints[1] = 0;
		ints[2] = 0;

		for (i=0; i<4; i++) {
			ints[1] = ints[1] | ((0xff & (init = data[i])) << (i*8));
			ints[2] = ints[2] | ((0xff & (init = data[4+i])) << (i*8));
		}

		result[0] = ints[1] / 100000.0;
		result[1] = ints[2] / 100000.0;
			
		half = 0;
		ri = 2;
		di = 8;
		
		while (di < dataSize) {
			ints[0] = ints[1];
			ints[1] = ints[2];
			if (di == (dataSize - 1) && half == 1) {
				if ((data[di] & 0xf) != 0x8) {
					break;
				}
			}
			decodeInt(data, &di, &half, &ints[2]);
			
			extrapol = ints[1] + (ints[1] - ints[0]);
			y = extrapol + ints[2];
			//printf("%d %d,   extrapol: %d    diff: %d \n", ints[0], ints[1], extrapol, ints[2]);
			result[ri++] 	= y / 100000.0;
			ints[2] 		= y;
		}
		result.resize(ri);
	} catch (...) {
		cerr << "DECODE ERROR" << endl;
		cerr << "i: " << i << endl;
		cerr << "ri: " << ri << endl;
		cerr << "resultSize: " << result.size() << endl;
		cerr << "di: " << di << endl;
		cerr << "half: " << half << endl;
		cerr << "dataSize: " << dataSize << endl;
		cerr << "ints[]: " << ints[0] << ", " << ints[1] << ", " << ints[2] << endl;
		cerr << "extrapol: " << extrapol << endl;
		cerr << "y: " << y << endl;

		for (i = di - 3; i < min(di + 3, dataSize); i++) {
			cerr << "data[" << i << "] = " << data[i];
		}
		cerr << endl;
	}
}

void MSNumpress::decodeCount(
		std::vector<unsigned char> &data,  
		std::vector<double> &result
) {
	size_t i, ri;
	int count;
	//double d;
	size_t di;
	size_t dataSize = data.size();
	int half;

	try {
		half = 0;
		ri = 0;
		di = 0;
		
		while (di < dataSize) {
			if (di == (dataSize - 1) && half == 1) {
				if ((data[di] & 0xf) != 0x8) {
					break;
				}
			}
			decodeInt(data, &di, &half, &count);
			
			//printf("count: %d \n", count);
			result[ri++] 	= count;
		}
		result.resize(ri);
	} catch (...) {
		cerr << "DECODE ERROR" << endl;
		cerr << "ri: " << ri << endl;
		cerr << "resultSize: " << result.size() << endl;
		cerr << "di: " << di << endl;
		cerr << "half: " << half << endl;
		cerr << "dataSize: " << dataSize << endl;
		cerr << "count: " << count << endl;

		for (i = di - 3; i < min(di + 3, dataSize); i++) {
			cerr << "data[" << i << "] = " << data[i];
		}
		cerr << endl;
	}
}




double TWO_BYTE_FLOAT_FIXED_POINT = 3000.0;
void MSNumpress::encode2ByteFloat(
		const double *data, 
		size_t dataSize, 
		unsigned char *result, 
		size_t *resultByteCount
) {
	size_t i, ri;
	unsigned short fp;
	ri = 0;

	for (i=0; i<dataSize; i++) {
		fp = log(data[i]) * TWO_BYTE_FLOAT_FIXED_POINT + 0.5;
		
		result[ri++] = fp & 0xff;
		result[ri++] = fp >> 8;
	}
	*resultByteCount = ri;
}



void MSNumpress::decode2ByteFloat(
		std::vector<unsigned char> &data,  
		std::vector<double> &result
) {
	size_t i, ri;
	size_t dataSize = data.size();
	unsigned short fp;
	ri = 0;

	result.resize(dataSize / 2);
	
	for (i=0; i<dataSize; i+=2) {
		fp = data[i] | (data[i+1] << 8);
		result[ri++] = exp(fp / TWO_BYTE_FLOAT_FIXED_POINT);
	}
}


} // namespace numpress
} // namespace ms