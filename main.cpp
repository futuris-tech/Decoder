#include <iostream>
#include <iomanip>
#include <fstream>
#include <limits>
#include <cstring>
#include <algorithm>
#include <array>
using namespace std;

const uint AlphSizeEn = 'z' - 'a' + 1;
const uint AlphSizeRu = 32;

char move_en(char c) {
	if (c < 'a' || c > 'z') return c;
	if (c == 'z') return 'a';
	return c + 1;
}

uint get_utf8(char*& _ptr) {
	auto& ptr = (uint8_t*&)_ptr;
	uint c = *ptr++;
	if (!(c & 0x80)) return c;
	uint8_t i = c;
	uint shift = 8;
	do {
		c |= *ptr++ << shift;
		shift += 8;
		i <<= 1;
	} while (i & 0x40);
	return c;
}
void put_utf8(char*& ptr, uint c) {
	do *ptr++ = c;
	while (c >>= 8);
}
constexpr uint move_utf8(uint c, uint shift) {
	auto to_code = [](uint c) -> int {
		switch (c & 0xff) {
		case 0xd0:
			c >>= 8;
			c -= 0x90;
			if (c < 0x30)
				return c;
			return -1;
		case 0xd1:
			c >>= 8;
			c -= 0x80;
			if (c < 0x10)
				return c + 0x30;
		default:
			return -1;
		}
	};
	auto to_utf = [](int c) -> int {
		if (c < 0x30)
			return 0x90d0 + (c << 8);
		else
			return 0x50d1 + (c << 8);
	};

	auto code = to_code(c);
	if (code == -1) return c;
	auto type = code & 0xffffffe0;
	code += shift;
	code = type | (code & 0x1f);
	return to_utf(code);
}

int decode_email(char* str) {
	const auto keys = {
		"com",
		"org",
		"net",
		"biz",
		"info"
	};

	for (uint i = 0; i < AlphSizeEn; i++) {
		for (auto key : keys) {
			auto found = strstr(str, key);
			if (!found) continue;
			auto next = found[strlen(key)];
			if (!next) return i;
		}

		for (auto ptr = str; *ptr; ptr++)
			*ptr = move_en(*ptr);
	}
	return -1;
}
int decode_addr(char* str) {
	for (uint i = 0; i < AlphSizeRu; i++) {
		auto found = strstr(str, (const char*)u8"кв.");
		if (found && found[5] >= '1' && found[5] <= '9')
			return i;

		for (auto ptr = str; *ptr; ) {
			auto ptr2 = ptr;
			auto utf = get_utf8(ptr2);
			utf = move_utf8(utf, 1);
			put_utf8(ptr, utf);
		}
	}
	return -1;
}

class phone_database {
	struct hash_t {
		char hash[20];
		bool operator<(const hash_t& b) const {
			return memcmp(hash, b.hash, 20) < 0;
		}
	};
	struct item_t : hash_t {
		uint value;
	};
	array<item_t, 1000> data;

	char from_hex(char c) {
		c -= '0';
		if (c < 10) return c;
		return c + '0' - 'a' + 10;
	}
	void sha1_unformat(char* dst, const char* src) {
		for (uint i = 20; i; i--) {
			uint8_t c = from_hex(*src++) << 4;
			c |= from_hex(*src++);
			*dst++ = c;
		}
	}
public:
	phone_database() {
		ifstream input("/home/ivan/Downloads/hacked.txt", ios::binary);
		char hash_str[40];
		for (auto& item : data) {
			input.read(hash_str, 40);
			sha1_unformat(item.hash, hash_str);
			input.ignore(1);
			if (input.get() != '8') throw;
			if (input.get() != '9') throw;
			input >> item.value;
			input.ignore(1);
		}
		sort(data.begin(), data.end());
	}

	auto decode(const char* hash_str) {
		hash_t hash;
		sha1_unformat(hash.hash, hash_str);
		auto ptr = lower_bound(data.begin(), data.end(), hash);
		if (memcmp(ptr->hash, hash.hash, 20)) throw;
		return ptr->value + 89000000000;
	}
};

int main() {
	ifstream input("/home/ivan/Downloads/student_v.1.11.csv", ios::binary);
	ofstream output("../decoded_11.csv", ios::binary);

	char buffer[256];
	input.getline(buffer, 256, '\n');
	output << buffer << '\n';
	
	phone_database pd;
	while (input.peek() != EOF) {
		char phone[41], email[128], addr[256];
		input.getline(phone, sizeof(phone), ';');
		input.getline(email, sizeof(email), ';');
		input.getline(addr, sizeof(addr));

		auto phone_value = pd.decode(phone);

		auto email_shift = decode_email(email);
		if (email_shift == -1) throw;
		
		auto addr_shift = decode_addr(addr);
		if (addr_shift == -1) throw;
		
		cout << email_shift << '\t' << addr_shift << '\n';
		output
			<< phone_value << ';'
			<< email << ';'
			<< addr << ';'
			<< email_shift << ';'
			<< addr_shift << '\n';
	}

	return 0;
}