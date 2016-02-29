//#define COMPRESSION 1 : libbz2-dev
//#define UNCOMPRESS 2 : libbz2-dev
//#define ENCRYPTION 3
//#define DECRYPTION 4



int computing_service(unsigned int computing_serv_type, char *processed_payload, unsigned int *processed_length, char *original_payload, unsigned int ori_length)
{
	//int comprLen=200000;
	int result;
	click_chatter("Service type: %d", computing_serv_type);
	switch (computing_serv_type)
	{
		case 1: // compression
			/*
			int BZ2_bzBuffToBuffCompress( char*  dest,
								unsigned int* destLen,
								char*         source,
								unsigned int  sourceLen,
								int           blockSize100k,
								int           verbosity,
								int           workFactor );
			*/
		    *processed_length=2000000; // maximum size of the zipped chunk
		    //click_chatter("%d", ori_length);
			result=BZ2_bzBuffToBuffCompress(processed_payload, processed_length, original_payload, ori_length, 5, 0, 30);
			if (result != BZ_OK)
				return -1;
			click_chatter("*** compressed size: %d ***", *processed_length);
			break;
		case 2: // uncompress
			/*
			int BZ2_bzBuffToBuffDecompress( char*         dest,
                                unsigned int* destLen,
                                char*         source,
                                unsigned int  sourceLen,
                                int           small,
                                int           verbosity );
            */
            *processed_length=2000000; // maximum size of the uncompressed chunk
			result=BZ2_bzBuffToBuffDecompress(processed_payload, processed_length, original_payload, ori_length, 0, 0);
			if (result != BZ_OK)
				return -1;
			click_chatter("*** uncompressed size: %d ***", *processed_length);
			break;
		case 3: // encrytion
			*processed_length = ori_length;
			for(unsigned int ii=0;ii<ori_length;ii++)
				processed_payload[ii] = original_payload[ii] ^ 3;
			break;
		case 4: // decryption
			*processed_length = ori_length;
			for(unsigned int ii=0;ii<ori_length;ii++)
				processed_payload[ii] = original_payload[ii] ^ 3;
			break;
		default: 
			break;
	}
	return 1;
}
