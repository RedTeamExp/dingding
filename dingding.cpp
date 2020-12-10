#include <Windows.h>
#include <stdio.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <string>
#include <iostream>
using namespace std;
#include <intrin.h>

#define SHA1_LEN 20
#define AES_BLOCK_SIZE (0x10)

wstring Ascii2Unicode(const string& str) {

	int unicodeLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);

	wchar_t* pUnicode = (wchar_t*)malloc(sizeof(wchar_t) * unicodeLen);

	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, pUnicode, unicodeLen);
	wstring ret_str = pUnicode;
	free(pUnicode);
	return ret_str;
}
string	Unicode2Ascii(const wstring& wstr) {

	int ansiiLen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);

	char* pAssii = (char*)malloc(sizeof(char) * ansiiLen);

	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
	string ret_str = pAssii;
	free(pAssii);
	return ret_str;
}

void dingding_AES_decrypt(const unsigned char* in, unsigned char* out, unsigned int len,
	const AES_KEY* key)
{
	int block_num = len / AES_BLOCK_SIZE;
	for (int i = 0; i < block_num; i++)
	{
		AES_decrypt(&in[i * AES_BLOCK_SIZE], &out[i * AES_BLOCK_SIZE], key);
	}
}

void normal_key_generate(std::string& key)
{
	int info[4] = { 0 };
	__cpuid(info, 0);

	char szInfo[100] = { 0 };
	snprintf(szInfo, sizeof(szInfo), "%d%d%d%d", info[0], info[1], info[2], info[3]);
	key.append(szInfo);
}

void user_key_generate(std::string& key, const char* ding_id)
{
	unsigned char md[16] = { 0 };
	char ascii_md[32 + 1] = { 0 };

	MD5_CTX ctx = { 0 };
	MD5_Init(&ctx);
	MD5_Update(&ctx, ding_id, strlen(ding_id));
	MD5_Final(md, &ctx);
	for (int i = 0; i < 16; i++)
	{
		sprintf(&ascii_md[i * 2], "%02x", md[i]);
	}

	key.append(ascii_md);
	printf(ascii_md);
}

//DbCrypt storage.db 
// DbCrypt dingtalk.db user ding_id
int wmain(int argc, wchar_t* argv[])
{
	bool is_user_db = false;
	if (0 == wcscmp(argv[2], L"user"))
		is_user_db = true;

	std::string my_testKey;
	//printf("%s", my_testKey);
	if (is_user_db)
	{
		wstring wstr = argv[3];
		printf("111");
		string	ding_id = Unicode2Ascii(wstr);
		//printf("%s",ding_id);
		user_key_generate(my_testKey, ding_id.c_str());
	}
	else
	{
		printf("222");
		normal_key_generate(my_testKey);
	}
	AES_KEY key = { 0 };

	const char* testKey = my_testKey.c_str();

	int ret = AES_set_decrypt_key((const unsigned  char*)testKey, 128, &key);
	if (ret < 0)
	{
		printf("AES_set_decrypt_key fial\n");
		return ret;
	}
	wchar_t* infile = argv[1];
	wchar_t outfile[MAX_PATH] = { 0 };
	printf("数据库已解密!输出dingtalk.db_plain.db");
	wsprintf(outfile, L"%s_plain.db", infile);

	FILE* fpdb = _wfopen(infile, L"rb+");
	if (!fpdb)
	{
		return 0;
	}
	fseek(fpdb, 0, SEEK_END);
	long nFileSize = ftell(fpdb);
	fseek(fpdb, 0, SEEK_SET);
	unsigned char* pDbBuffer = new unsigned char[nFileSize];
	fread(pDbBuffer, 1, nFileSize, fpdb);
	fclose(fpdb);

	int pageSize = 0x1000;
	int blockCnt = nFileSize / pageSize;

	unsigned char* output = (unsigned char*)malloc(pageSize);

	for (int i = 0; i < blockCnt; i++)
	{
		unsigned char* input = &pDbBuffer[i * pageSize];
		memset(output, 0, pageSize);

		dingding_AES_decrypt(input, output, pageSize, &key);
		FILE* fp = _wfopen(outfile, L"ab+");
		{
			fwrite(output, 1, pageSize, fp);
			fclose(fp);
		}

	}
	free(output);
}