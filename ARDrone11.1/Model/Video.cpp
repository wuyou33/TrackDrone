#include "Video.h"

Video::Video()
{
	nb_gob = 15;
	nb_mcblock = 20;

	quit=false;
	
	iplFrontImage = cvCreateImage( cvSize(320, 240 ), IPL_DEPTH_8U, 3 );
	iplBottomImage = cvCreateImage( cvSize(176, 144 ), IPL_DEPTH_8U, 3 );

	mutexImage= SDL_CreateMutex();
}	


IplImage* Video::getFrame(int type){
	if(type==FRONTCAMERA) return cvCloneImage(iplFrontImage);
	if(type==BOTTOMCAMERA) return cvCloneImage(iplBottomImage);
}

void Video::saveFrame(const char* filename,int type,bool color){

	mutexP();

	if(color){
		if(type==FRONTCAMERA)
			cvSaveImage(filename,iplFrontImage);
		if(type==BOTTOMCAMERA)
			cvSaveImage(filename,iplBottomImage);
	}else{
		if(type==FRONTCAMERA){
			IplImage* frameGray = cvCreateImage( cvSize(320, 240 ), IPL_DEPTH_8U, 1 );
			cvCvtColor(iplFrontImage,frameGray,CV_RGB2GRAY);
			cvSaveImage(filename,frameGray);
		}
		if(type==BOTTOMCAMERA){
			IplImage* frameGray = cvCreateImage( cvSize(176, 144 ), IPL_DEPTH_8U, 1 );
			cvCvtColor(iplBottomImage,frameGray,CV_RGB2GRAY);
			cvSaveImage(filename,frameGray);
		}
	}
	mutexV();
}


void Video::mutexP(){

	SDL_mutexP(mutexImage);
}

void Video::mutexV(){
	SDL_mutexV(mutexImage);
}

void Video::destroyMutex(){
	SDL_DestroyMutex(mutexImage);
}

void Video::get_screen()
{
	int sizeofbin = length << 3, i, j, k;
	char* bin;
	CvScalar pixel;

	bin = (char*) malloc( (sizeofbin + 1) * sizeof(char) );
	bin[sizeofbin] = 0;

	// Binarization of message received
	toBinary(bin, length >> 2);

	// RLE - Huffman
	if ( decode_image(bin, strlen(bin), &nb_gob, &nb_mcblock) == 0 )
	{
		for( i = 0 ; i < nb_gob ; i++ )
		{
			for( j = 0 ; j < nb_mcblock ; j++ )
			{
				for( k = 0 ; k < NB_BLOCK ; k++ )
				{
					// Zig-zag ordering
					zigzag_ordering(&group[i].macrob[j].bl[k]);

					// Dequantification of the matrix
					dequantification(group[i].macrob[j].bl[k].data);

					// IDCT
					idct(group[i].macrob[j].bl[k].data);
				}
			}
		}


		// init matrix
		if( nb_gob != NB_GOB_MAX )
		{
			for( i = 0 ; i < 240 ; i++ )
			{
				for( j = 0 ; j < 320 ; j++ )
				{ matrice[i][j].r = 0; matrice[i][j].g = 0; matrice[i][j].b = 0; }
			}
		}
		
		// creation of the RGB image
		if( nb_gob == 9 )
		get_matrice_from_Picture((Matrice_rgb*) &matrice[48][72], nb_gob, nb_mcblock);
		else
		get_matrice_from_Picture((Matrice_rgb*) &matrice[0][0], nb_gob, nb_mcblock);

		mutexP();	
		if( nb_gob == 9 ){
			for( i = 72 ; i < 72+176 ; i++ )
				for( j = 48 ; j < 48+144 ; j++ ){
					pixel = cvScalar(matrice[j][i].b
							,matrice[j][i].g
							,matrice[j][i].r
							,0); //BGR
					
					cvSet2D(iplBottomImage,j-48,i-72,pixel);
			}
		}
		else{
			for( i = 0 ; i < 320 ; i++ )
				for( j = 0 ; j < 240 ; j++ ){
					pixel = cvScalar(matrice[j][i].b,matrice[j][i].g,matrice[j][i].r,0); //BGR
					cvSet2D(iplFrontImage,j,i,pixel);
			}
		}

		mutexV();

	}

	free(bin);
}

void Video::initializationCommunication(){

	communication = new Communication(VIDEO_PORT);
	communication->init();
}

void Video::dequantification(short* data){

	int i;
	for( i = 0 ; i < 64 ; i++ )
	{
		if( data[i] )
		data[i] *= table_iquant[i];
	}
}


void Video::zigzag_ordering(Block* bl){

	int i;
	for( i = 0 ; i < 64 ; i++ )
	bl->data[table_zigzag[i]] = bl->tmp[i];
}

void Video::toBinary(char* bin, int sizeofbuf){

	unsigned int mask;
	int i;
	for( i = 0 ; i < sizeofbuf ; i++ )
	{
		mask = 0x80000000;

		while( mask )
		{
			if( mask & msg[i] )
			*bin = '1';
			else
			*bin = '0';

			bin++;
			mask >>= 1;
		}
	}
}

int Video::get_int_from_binary(char* bin, int* index, int size, int nb_bits){

	int i;
	int ret = 0;
	int lim = *index + nb_bits;
	unsigned int mask = 1;

	mask <<= (nb_bits - 1);

	for( i = *index ; i < lim ; i++ )
	{
		if( bin[i] == '1' )
		ret += mask;

		mask >>= 1;
	}

	(*index) += nb_bits;

	return ret;
}

int Video::decode_block(Block* bl, char* bin, int size, int* index, int DC)
{
	int i, zero_counter, decoding = 1;
	short x = 0xFC00, MASK = 0x200;
	unsigned short mask;

	bl->nb_data = 0;
	for( i = 0 ; i < 64 ; i++ )
	bl->tmp[i] = 0;

	for( i = *index ; i < *index + 10 ; i++ )
	{
		if( bin[i] == '1' )
		x |= MASK;

		MASK = MASK >> 1;
	}

	(*index) += 10;
	bl->tmp[bl->nb_data] = (short)x;
	bl->nb_data++;

	while( DC && decoding )
	{
		zero_counter = 0;

		while( bin[*index] != '1' )
		{
			zero_counter++;
			(*index)++;
		}

		if( zero_counter )
		{
			(*index)++;

			zero_counter = 1 << (zero_counter-1);
			mask = zero_counter >> 1;

			while( mask )
			{
				if( bin[*index] == '1' )
				zero_counter |= mask;

				(*index)++;
				mask = mask >> 1;
			}

			while( zero_counter > 0 )
			{
				bl->tmp[bl->nb_data] = 0;
				bl->nb_data += 1;
				zero_counter--;
			}
		}

		else
		(*index)++;

		while( bin[*index] != '1' )
		{
			zero_counter++;
			(*index)++;
		}

		if( !zero_counter )
		{
			(*index)++;

			if( bin[*index] == '1' )
			bl->tmp[bl->nb_data] = -1;
			else
			bl->tmp[bl->nb_data] = 1;

			bl->nb_data += 1;
			(*index)++;
		}

		else if( zero_counter != 1 )
		{
			(*index) ++;

			bl->tmp[bl->nb_data] = 1 << (zero_counter - 1);
			mask = bl->tmp[bl->nb_data] >> 1;

			while( mask )
			{
				if( bin[*index] == '1' )
				bl->tmp[bl->nb_data] |= mask;

				(*index)++;
				mask = mask >> 1;
			}

			if( bin[*index] == '1' )
			bl->tmp[bl->nb_data] = -bl->tmp[bl->nb_data];

			bl->nb_data += 1;

			(*index)++;
		}

		else
		{
			decoding = 0;
			(*index)++;
		}
	}

	return 0;
}

int Video::decode_macroblock(Macroblock* mb, char* bin, int size, int* index)
{
	int i;
	int mask = 0x01;

	mb->mbc = get_int_from_binary(bin, index, size, 1);

	if( ! (mb->mbc) )
	{
		mb->mbdesc = get_int_from_binary(bin, index, size, 8);

		if( (mb->mbdesc >> 6) & 1 )
		{
			mb->mbdiff = get_int_from_binary(bin, index, size, 2);

			if( mb->mbdiff == 1 )
			mb->mbdiff = -2;
			else
			mb->mbdiff--;
		}
		else
		mb->mbdiff = 0;

		for( i = 0 ; i < NB_BLOCK ; i++ )
		{
			decode_block(&(mb->bl[i]), bin, size, index, mb->mbdesc & mask);
			mask = mask << 1;
		}
	}
	else
	{
		for( i = 0 ; i < NB_BLOCK ; i++ )
		{
			mb->bl[i].nb_data = 0;
		}
	}

	return 0;
}

int Video::decode_gob(char* bin, int size, int* index, int* nb_mcblock)
{
	int ret = 0, i = 0;
	group[numofgob].numofmacroblock = 0;

	if( numofgob )
	{
		if( *index % 8 )
		*index += (8 - *index%8);

		group[numofgob].code_start = get_int_from_binary(bin, index, size, 22);
		group[numofgob].quantificateur = get_int_from_binary(bin, index, size, 5);

		if( group[numofgob].code_start < 32 || group[numofgob].code_start > 50 )
		return -1;
	}
	else
	{
		group[numofgob].code_start = 32;
		group[numofgob].quantificateur = 31;
	}

	for( i = 0 ; i < *nb_mcblock ; i++ )
	{
		ret = decode_macroblock(&(group[numofgob].macrob[group[numofgob].numofmacroblock]), bin, size, index);
		group[numofgob].numofmacroblock++;
	}

	return ret;
}

int Video::decode_image(char* bin, int size, int* nb_gob, int* nb_mcblock)
{
	int index = 0;
	int ret = 0;
	int i;

	code_start = get_int_from_binary(bin, &index, size, 22);
	format = get_int_from_binary(bin, &index, size, 2);
	resolution = get_int_from_binary(bin, &index, size, 3);
	type = get_int_from_binary(bin, &index, size, 3);
	quantificateur = get_int_from_binary(bin, &index, size, 5);
	number = get_int_from_binary(bin, &index, size, 32);
	numofgob = 0;

	for( i = 0 ; i < *nb_gob ; i++ )
	{
		ret = decode_gob(bin, size, &index, nb_mcblock);
		numofgob++;

		if( ret == -1 )
		{
			if( *nb_gob == 15 )
			{
				*nb_gob = 9;
				*nb_mcblock = 11;
			}
			else
			{
				*nb_gob = 15;
				*nb_mcblock = 20;
			}

			return -1;
		}
	}

	return 0;
}

void Video::idct(short* in)
{
  INT32 tmp0, tmp1, tmp2, tmp3;
  INT32 tmp10, tmp11, tmp12, tmp13;
  INT32 z1, z2, z3, z4, z5;
  int* wsptr;
  int* outptr;
  const short* inptr;
  int ctr;
  int workspace[DCTSIZE2];
  int data[DCTSIZE2];

  inptr = in;
  wsptr = workspace;
  for (ctr = DCTSIZE; ctr > 0; ctr--)
  {
    if( inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*2] == 0 &&
        inptr[DCTSIZE*3] == 0 && inptr[DCTSIZE*4] == 0 &&
        inptr[DCTSIZE*5] == 0 && inptr[DCTSIZE*6] == 0 &&
        inptr[DCTSIZE*7] == 0 ) {

      int dcval = inptr[DCTSIZE*0] << PASS1_BITS;

      wsptr[DCTSIZE*0] = dcval;
      wsptr[DCTSIZE*1] = dcval;
      wsptr[DCTSIZE*2] = dcval;
      wsptr[DCTSIZE*3] = dcval;
      wsptr[DCTSIZE*4] = dcval;
      wsptr[DCTSIZE*5] = dcval;
      wsptr[DCTSIZE*6] = dcval;
      wsptr[DCTSIZE*7] = dcval;

      inptr++;
      wsptr++;
      continue;
    }

    z2 = inptr[DCTSIZE*2];
    z3 = inptr[DCTSIZE*6];

    z1 = MULTIPLY(z2 + z3, FIX_0_541196100);
    tmp2 = z1 + MULTIPLY(z3, - FIX_1_847759065);
    tmp3 = z1 + MULTIPLY(z2, FIX_0_765366865);

    z2 = inptr[DCTSIZE*0];
    z3 = inptr[DCTSIZE*4];

    tmp0 = (z2 + z3) << CONST_BITS;
    tmp1 = (z2 - z3) << CONST_BITS;

    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;

    tmp0 = inptr[DCTSIZE*7];
    tmp1 = inptr[DCTSIZE*5];
    tmp2 = inptr[DCTSIZE*3];
    tmp3 = inptr[DCTSIZE*1];

    z1 = tmp0 + tmp3;
    z2 = tmp1 + tmp2;
    z3 = tmp0 + tmp2;
    z4 = tmp1 + tmp3;
    z5 = MULTIPLY(z3 + z4, FIX_1_175875602);

    tmp0 = MULTIPLY(tmp0, FIX_0_298631336);
    tmp1 = MULTIPLY(tmp1, FIX_2_053119869);
    tmp2 = MULTIPLY(tmp2, FIX_3_072711026);
    tmp3 = MULTIPLY(tmp3, FIX_1_501321110);
    z1 = MULTIPLY(z1, - FIX_0_899976223);
    z2 = MULTIPLY(z2, - FIX_2_562915447);
    z3 = MULTIPLY(z3, - FIX_1_961570560);
    z4 = MULTIPLY(z4, - FIX_0_390180644);

    z3 += z5;
    z4 += z5;

    tmp0 += z1 + z3;
    tmp1 += z2 + z4;
    tmp2 += z2 + z3;
    tmp3 += z1 + z4;

    wsptr[DCTSIZE*0] = (int) DESCALE(tmp10 + tmp3, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*7] = (int) DESCALE(tmp10 - tmp3, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*1] = (int) DESCALE(tmp11 + tmp2, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*6] = (int) DESCALE(tmp11 - tmp2, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*2] = (int) DESCALE(tmp12 + tmp1, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*5] = (int) DESCALE(tmp12 - tmp1, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*3] = (int) DESCALE(tmp13 + tmp0, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*4] = (int) DESCALE(tmp13 - tmp0, CONST_BITS-PASS1_BITS);

    inptr++;
    wsptr++;
  }

  wsptr = workspace;
  outptr = data;
  for (ctr = 0; ctr < DCTSIZE; ctr++) {

    z2 = (INT32) wsptr[2];
    z3 = (INT32) wsptr[6];

    z1 = MULTIPLY(z2 + z3, FIX_0_541196100);
    tmp2 = z1 + MULTIPLY(z3, - FIX_1_847759065);
    tmp3 = z1 + MULTIPLY(z2, FIX_0_765366865);

    tmp0 = ((INT32) wsptr[0] + (INT32) wsptr[4]) << CONST_BITS;
    tmp1 = ((INT32) wsptr[0] - (INT32) wsptr[4]) << CONST_BITS;

    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;

    tmp0 = (INT32) wsptr[7];
    tmp1 = (INT32) wsptr[5];
    tmp2 = (INT32) wsptr[3];
    tmp3 = (INT32) wsptr[1];

    z1 = tmp0 + tmp3;
    z2 = tmp1 + tmp2;
    z3 = tmp0 + tmp2;
    z4 = tmp1 + tmp3;
    z5 = MULTIPLY(z3 + z4, FIX_1_175875602);

    tmp0 = MULTIPLY(tmp0, FIX_0_298631336);
    tmp1 = MULTIPLY(tmp1, FIX_2_053119869);
    tmp2 = MULTIPLY(tmp2, FIX_3_072711026);
    tmp3 = MULTIPLY(tmp3, FIX_1_501321110);
    z1 = MULTIPLY(z1, - FIX_0_899976223);
    z2 = MULTIPLY(z2, - FIX_2_562915447);
    z3 = MULTIPLY(z3, - FIX_1_961570560);
    z4 = MULTIPLY(z4, - FIX_0_390180644);

    z3 += z5;
    z4 += z5;

    tmp0 += z1 + z3;
    tmp1 += z2 + z4;
    tmp2 += z2 + z3;
    tmp3 += z1 + z4;

    outptr[0] = (tmp10 + tmp3) >> ( CONST_BITS+PASS1_BITS+3 );
    outptr[7] = (tmp10 - tmp3) >> ( CONST_BITS+PASS1_BITS+3 );
    outptr[1] = (tmp11 + tmp2) >> ( CONST_BITS+PASS1_BITS+3 );
    outptr[6] = (tmp11 - tmp2) >> ( CONST_BITS+PASS1_BITS+3 );
    outptr[2] = (tmp12 + tmp1) >> ( CONST_BITS+PASS1_BITS+3 );
    outptr[5] = (tmp12 - tmp1) >> ( CONST_BITS+PASS1_BITS+3 );
    outptr[3] = (tmp13 + tmp0) >> ( CONST_BITS+PASS1_BITS+3 );
    outptr[4] = (tmp13 - tmp0) >> ( CONST_BITS+PASS1_BITS+3 );

    wsptr += DCTSIZE;
    outptr += DCTSIZE;
  }

  for(ctr = 0; ctr < DCTSIZE2; ctr++)
	in[ctr] = 255 - ((0-data[ctr]) - 128);

}

void Video::get_matrice(Matrice_rgb* ret, short* luma, short* blue, short* red, int* index)
{
	int x = 0;
	int i;
	int ind = *index;


	for( i = 0 ; i < 16 ; i++ )
	{
		if( i != 0 && i % 4 == 0 )
		{
			ind += 4;
			x += 8;
		}

		// RED
		ret[x].r = luma[x] + 1.402 * ( red[ind] - 128 );
		ret[x+1].r = luma[x+1] + 1.402 * ( red[ind] - 128 );
		ret[x+8].r = luma[x+8] + 1.402 * ( red[ind] - 128 );
		ret[x+9].r = luma[x+9] + 1.402 * ( red[ind] - 128 );

		// GREEN
		ret[x].g = luma[x] - 0.34414 * ( blue[ind] - 128 ) - 0.71414 * ( red[ind] - 128 );
		ret[x+1].g = luma[x+1] - 0.34414 * ( blue[ind] - 128 ) - 0.71414 * ( red[ind] - 128 );
		ret[x+8].g = luma[x+8] - 0.34414 * ( blue[ind] - 128 ) - 0.71414 * ( red[ind] - 128 );
		ret[x+9].g = luma[x+9] - 0.34414 * ( blue[ind] - 128 ) - 0.71414 * ( red[ind] - 128 );

		// BLUE
		ret[x].b = luma[x] + 1.772 * ( blue[ind] - 128 );
		ret[x+1].b = luma[x+1] + 1.772 * ( blue[ind] - 128 );
		ret[x+8].b = luma[x+8] + 1.772 * ( blue[ind] - 128 );
		ret[x+9].b = luma[x+9] + 1.772 * ( blue[ind] - 128 );

		x += 2;
		ind++;
	}

	if( ! *index )
	*index = 4;
	else if( *index == 4 )
	*index = 32;
	else if( *index == 32 )
	*index = 36;
	else if( *index == 36 )
	*index = 0;

	for( i = 0 ; i < 64 ; i++ )
	{
		if( ret[i].r > 255 ) ret[i].r = 255;
		if( ret[i].g > 255 ) ret[i].g = 255;
		if( ret[i].b > 255 ) ret[i].b = 255;
		if( ret[i].r < 0 ) ret[i].r = 0;
		if( ret[i].g < 0 ) ret[i].g = 0;
		if( ret[i].b < 0 ) ret[i].b = 0;
	}
}

void Video::get_matrice_from_Picture(Matrice_rgb* mat, int nb_gob, int nb_mcblock)
{
    int i, j, k, l, m, index = 0;
	int x = 0, y = 0;
    Matrice_rgb color_matrice[64];

    for( i = 0 ; i < nb_gob ; i++ )
    {
        for( j = 0 ; j < nb_mcblock ; j++ )
        {
            for( k = 0 ; k < 4 ; k++ )
            {
                get_matrice(color_matrice, group[i].macrob[j].bl[k].data, group[i].macrob[j].bl[4].data, group[i].macrob[j].bl[5].data, &index);

                for( l = 0 ; l < 8 ; l++ )
                {
                    for( m = 0 ; m < 8 ; m++ )
                    {
                        mat[y+x].r = color_matrice[(l<<3)+m].r;
                        mat[y+x].g = color_matrice[(l<<3)+m].g;
                        mat[y+x].b = color_matrice[(l<<3)+m].b;

                        x++;
                    }

                    x-=8;
					y+=320;
                }

 				if( k == 0 || k == 2 ) { y-=2560; x+=8; }
                if( k == 1 ) { x-=8; }
                if( k == 3 ) { y-=5120; x+=8; }
            }
        }

        y+=5120;
        x = 0;
    }
}



void Video::disconnection(){

	communication->closeSocket();

}

bool Video::getQuit(){return quit;}

void Video::shutDown(){
	quit=true;
}

/******************************** THREAD ********************************/
int threadVideo(void *data){

	Video* video = (Video*)data;

	video->initializationCommunication();
	video->communication->wakeUpPort();

	while(!video->getQuit())
	{
		video->communication->selectReceiveData((void*)video->msg, &video->length, (TAILLE_MAX_VIDEO*4)-4, TIMER);
		
		if( video->length < 1 )
		video->communication->wakeUpPort();
		else
		video->get_screen();
		
	}
	

	printf("END THREAD VIDEO\n");

	return 0;
}

