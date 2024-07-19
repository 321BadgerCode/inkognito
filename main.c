// Badger
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <regex.h>

typedef unsigned char u8;

typedef struct {
	u8* pixels;
	int width;
	int height;
} bitmap;

unsigned int pixel_min = 0x80;

void save_bitmap_to_file(bitmap* bmp, const char* file_name) {
	FILE* file = fopen(file_name, "wb");
	if(file) {
		fprintf(file, "P5\n%d %d\n255\n", bmp->width, bmp->height);

		fwrite(bmp->pixels, bmp->width * bmp->height, 1, file);

		fclose(file);
	}
}

bitmap get_bitmap(const char* file_name) {
	bitmap bmp;
	FILE* file = fopen(file_name, "rb");
	size_t _tmp;

	if(file){
		char magic_number[3];
		_tmp = fscanf(file, "%2s", magic_number);
		if(strcmp(magic_number, "P5") != 0){
			fprintf(stderr, "Error: Invalid bitmap format.\n");
			fclose(file);
			return bmp;
		}

		_tmp = fscanf(file, "%d %d", &bmp.width, &bmp.height);
		int max_value;
		_tmp = fscanf(file, "%d", &max_value);
		if(max_value != 255){
			fprintf(stderr, "Error: Only 8-bit bitmaps are supported.\n");
			fclose(file);
			return bmp;
		}

		bmp.pixels = (u8*)malloc(bmp.width * bmp.height);
		if(bmp.pixels == NULL){
			fprintf(stderr, "Error: Memory allocation failed.\n");
			fclose(file);
			return bmp;
		}

		char padding;
		_tmp = fread(&padding, 1, 1, file);

		_tmp = fread(bmp.pixels, bmp.width * bmp.height, 1, file);

		fclose(file);
	}
	else{
		fprintf(stderr, "Error: Could not open file %s.\n", file_name);
	}

	return bmp;
}

// FIXME: get to work for all string lengths (some tests didn't pass: "this is a test string")
char* move_chars(char* input, int move) {
	int len = strlen(input);
	char* output = (char*)malloc(len + 1);
	for (int i = 0; i < len; i++) {
		output[i] = input[(i + move) % len];
	}
	return output;
}
char* reverse(char* input) {
	int len = strlen(input);
	char* output = (char*)malloc(len + 1);
	for (int i = 0; i < len; i++) {
		output[i] = input[len - i - 1];
	}
	return output;
}

// FIXME: get to work. needs to be symmetric with rotate value, so can use negative for decryption. fn(1) = fn(-fn(1))
char* rotate_ascii(char* input, int rotate) {
	int len = strlen(input);
	char* output = (char*)malloc(len + 1);
	for (int i = 0; i < len; i++) {
		output[i] = (input[i] + rotate) % (int)'~';
	}
	return output;
}

int get_sum(char* input) {
	int sum = 0;
	for (int i = 0; i < strlen(input); i++) {
		sum += input[i];
	}
	return sum;
}

char* get_file_content(const char* file_name) {
	FILE* file = fopen(file_name, "rb");
	if (file) {
		fseek(file, 0, SEEK_END);
		long size = ftell(file);
		rewind(file);
		char* buffer = (char*)malloc(size + 1);
		size_t _tmp = fread(buffer, size, 1, file);
		fclose(file);
		buffer[size] = 0;
		for (int i = 0; i < size; i++) {
			if (buffer[i] == '\n') {
				buffer[i] = '\\';
				buffer[i + 1] = 'n';
			}
		}
		return buffer;
	}
	return NULL;
}

char* replace_str(char* str, char* orig, char* rep) {
	char* ret = (char*)malloc(4096);
	char* p;
	int i = 0;
	int orig_len = strlen(orig);
	int rep_len = strlen(rep);
	while ((p = strstr(str, orig)) != NULL) {
		strncpy(&ret[i], str, p - str);
		i += p - str;
		strncpy(&ret[i], rep, rep_len);
		i += rep_len;
		str = p + orig_len;
	}
	strcpy(&ret[i], str);
	return ret;
}

long long time_ms(void){
	struct timeval tv;

	gettimeofday(&tv,NULL);
	return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}
u8 get_random_u8(u8 min, u8 max) {
	srand((unsigned int)time_ms());
	return (u8)(min + (rand() % (max - min + 1)));
}

char* get_base(u8 value, int base){
	char* hex = "0123456789ABCDEF";
	char* result = (char*)malloc(8);
	int i = 0;
	while(value > 0){
		result[i] = hex[value % base];
		value /= base;
		i++;
	}
	result[i] = 0;
	return result;
}

bitmap encode(const char* msg, u8 rot) {
	bitmap result;
	char* rot_bin = get_base(rot, 2);
	result.width = strlen(rot_bin);
	result.height = ceil((float)strlen(msg)/result.width) + 1;
	result.pixels = (u8*)malloc(result.width * result.height);
	for(int i = 0; i < result.width; i++){
		result.pixels[i] = rot_bin[i] == '1' ? get_random_u8(pixel_min, 0xff) : get_random_u8(0x00, pixel_min - 1);
	}
	for(int i = 1; i < result.height; i++){
		for(int j = 0; j < result.width; j++){
			result.pixels[i * result.width + j] = msg[(i-1) * result.width + j];
		}
	}
	return result;
}

int get_pow(int base, int exp){
	int result = 1;
	for(int i = 0; i < exp; i++){
		result *= base;
	}
	return result;
}

int get_bin_val(bitmap* bmp, int start, int end){
	int result = 0;
	for(int i = start; i < end; i++){
		result += bmp->pixels[i] > pixel_min ? get_pow(2, i - start) : 0;
	}
	return result;
}

char* decode(bitmap* bmp){
	char* result = (char*)malloc(bmp->width * (bmp->height-1));
	u8 rot = get_bin_val(bmp, 0, bmp->width);
	for(int i = 1; i < bmp->height; i++){
		for(int j = 0; j < bmp->width; j++){
			result[(i-1) * bmp->width + j] = bmp->pixels[i * bmp->width + j];
		}
	}
	//result = move_chars(result, get_sum(result) % strlen(result));
	result = reverse(result);
	//result = rotate_ascii(result, -rot);
	return result;
}

bitmap xor_bitmap(bitmap* bmp1, bitmap* bmp2){
	bitmap result;
	result.width = bmp1->width;
	result.height = bmp1->height;
	result.pixels = (u8*)malloc(result.width * result.height);
	for(int i = 0; i < result.height; i++){
		for(int j = 0; j < result.width; j++){
			result.pixels[i * result.width + j] = bmp1->pixels[i * result.width + j] ^ bmp2->pixels[i * result.width + j];
		}
	}
	return result;
}

bitmap grayscale_bitmap(bitmap* bmp){
	bitmap result;
	result.width = bmp->width;
	result.height = bmp->height;
	result.pixels = (u8*)malloc(result.width * result.height);
	for(int i = 0; i < result.height; i++){
		for(int j = 0; j < result.width; j++){
			result.pixels[i * result.width + j] = bmp->pixels[i * result.width + j] > pixel_min ? 0xff : 0x00;
		}
	}
	return result;
}

bitmap default_mix_bmp(){
	bitmap mix_bmp;
	mix_bmp.width = 256;
	mix_bmp.height = 256;
	mix_bmp.pixels = (u8*)malloc(mix_bmp.width * mix_bmp.height);

	for (int y = 0; y < mix_bmp.height; y++) {
		for (int x = 0; x < mix_bmp.width; x++) {
			mix_bmp.pixels[y * mix_bmp.width + x] = (u8)(x ^ y);
		}
	}

	return grayscale_bitmap(&mix_bmp);
}

char* bitmap_to_base64(bitmap* bmp){
	char* digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	bitmap gray_bmp = grayscale_bitmap(bmp);
	size_t len = bmp->width * bmp->height / 6;
	char* result = (char*)malloc(len);
	for (int i = 0; i < len; i++) {
		u8 value = get_bin_val(&gray_bmp, i * 6, (i + 1) * 6);
		result[i] = digits[value];
	}
	return result;
}

bitmap base64_to_bitmap(char* base64){
	char* digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	bitmap result;
	result.width = 256;
	result.height = 256;
	result.pixels = (u8*)malloc(result.width * result.height);
	for(int i = 0; i < result.height / 6; i++){
		u8 value = strchr(digits, base64[i]) - digits;
		for(int j = 0; j < 6; j++){
			result.pixels[i * 6 + j] = value & (1 << j) ? 0xff : 0x00;
		}
	}
	return result;
}

bool is_text_file(const char* file_name) {
	regex_t regex;
	int reti;
	reti = regcomp(&regex, ".*\\.txt", 0);
	if (reti) {
		fprintf(stderr, "Could not compile regex\n");
		return false;
	}
	reti = regexec(&regex, file_name, 0, NULL, 0);
	if (!reti) {
		return true;
	}
	return false;
}

int main(int argc, char** argv){
	bool is_encode = true;
	char* msg = NULL;
	char* img_out = "./output.bmp";
	u8 rot = get_random_u8(0, 0xff);
	bitmap mix_bmp = default_mix_bmp();
	for(int i = 0; i < argc; i++){
		if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
			printf("Inkognito: A simple steganography tool\n");
			printf("Usage: %s [options]\n", argv[0]);
			printf("Options:\n");
			printf("-h|--help:\t\thelp\n");
			printf("--version:\t\tget version\n");
			printf("-e:\t\t\tencode [default]\n");
			printf("-d:\t\t\tdecode\n");
			printf("-i <message|filename>:\tinput string or a file\n");
			printf("-o <filename>:\t\toutput image file [default: ./output.bmp]\n");
			printf("-r <int>:\t\trotate ascii in input text [default: random]\n");
			printf("-m <seed|filename>:\tmix with specific base64 seed or image [optional]\n");
			printf("-p <int>:\t\tpixel minimum value to be true [default: 128]\n");
			return 0;
		}
		else if(strcmp(argv[i], "--version") == 0){
			printf("Inkognito v0.1\n");
			return 0;
		}
		else if(strcmp(argv[i], "-e") == 0){
			is_encode = true;
		}
		else if(strcmp(argv[i], "-d") == 0){
			is_encode = false;
		}
		else if(strcmp(argv[i], "-i") == 0){
			i++;
			if(i >= argc){
				printf("Invalid input string\n");
				return 0;
			}
			if (is_text_file(argv[i])) {
				msg = get_file_content(argv[i]);
			}
			else {
				msg = argv[i];
			}
		}
		else if(strcmp(argv[i], "-o") == 0){
			i++;
			if(i >= argc){
				printf("Invalid output file\n");
				return 0;
			}
			img_out = argv[i];
		}
		else if(strcmp(argv[i], "-r") == 0){
			i++;
			if(i >= argc){
				printf("Invalid rotate value\n");
				return 0;
			}
			rot = atoi(argv[i]);
		}
		else if(strcmp(argv[i], "-m") == 0){
			i++;
			if(i >= argc){
				printf("Invalid mix seed or file\n");
				return 0;
			}
			if (fopen(argv[i], "r") != NULL){
				mix_bmp = get_bitmap(argv[i]);
			}
			else{
				mix_bmp = base64_to_bitmap(argv[i]);
			}
			return 0;
		}
		else if(strcmp(argv[i], "-p") == 0){
			i++;
			if(i >= argc){
				printf("Invalid pixel minimum value\n");
				return 0;
			}
			pixel_min = atoi(argv[i]);
		}
	}

	if(is_encode){
		if(msg == NULL){
			printf("Invalid input string\n");
			return 0;
		}
		//msg = rotate_ascii(msg, rot);
		msg = reverse(msg);
		//msg = move_chars(msg, get_sum(msg) % strlen(msg));
		// TODO: delete testing code
		//printf(bitmap_to_base64(&mix_bmp));
		//printf("\n");
		bitmap bmp = encode(msg, rot);
		bmp = xor_bitmap(&bmp, &mix_bmp);
		save_bitmap_to_file(&bmp, img_out);
		free(bmp.pixels);
	}
	else{
		//FIXME: get base64_to_bitmap to work
		//bitmap tmp = base64_to_bitmap("AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA//////////////////////////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAAw/////////////////////AAAAAAAAAAAAAAAAAAAAA8////////////////////PAAAAAAAAAAAAAAAAAAAAA/////////////////////DAAAAAAAAAAAAAAAAAAAA");
		//save_bitmap_to_file(&tmp, "mix.bmp");
		bitmap bmp = get_bitmap(img_out);
		bmp = xor_bitmap(&bmp, &mix_bmp);
		char* result = decode(&bmp);
		printf("%s\n", replace_str(result, "\\n", "\n"));
	}

	free(mix_bmp.pixels);

	return 0;
}
// FIXME: make sure base64 stuff works