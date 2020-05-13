#ifndef H264_FILE_HPP
#define H264_FILE_HPP

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include <cstdint>
#include <cstdio>
#include <memory>
#include <cstring>

class H264File
{
public:
	H264File() {
		buffer_size_ = kMaxFrameSize;
		buffer_.reset(new char[buffer_size_]);
	}

	virtual ~H264File() {}

	bool Open(const char* pathname) {
		if (file_) {
			Close();
		}

		file_ = fopen(pathname, "rb");
		return (file_ != nullptr);
	}

	void Close() {
		if (file_) {
			fclose(file_);
			file_ = nullptr;
		}
	}

	int ReadFrame(char* in_buf, int in_buf_size, bool* end) {
		if (file_ == nullptr) {
			return -1;
		}

		int bytes_read = (int)fread(buffer_.get(), 1, buffer_size_, file_);
		if (bytes_read == 0) {
			fseek(file_, 0, SEEK_SET);
			frame_count_ = 0;
			bytes_read = (int)fread(buffer_.get(), 1, buffer_size_, file_);
			if (bytes_read == 0) {
				this->Close();
				return -1;
			}
		}

		bool is_find_start = false, is_find_end = false;
		int i = 0, start_code = 3;
		*end = false;

		for (i = 0; i < bytes_read - 5; i++) {
			if (buffer_.get()[i] == 0 && buffer_.get()[i + 1] == 0 && 
				buffer_.get()[i + 2] == 1) {
				start_code = 3;
			}
			else if (buffer_.get()[i] == 0 && buffer_.get()[i + 1] == 0 &&
				buffer_.get()[i + 2] == 0 && buffer_.get()[i + 3] == 1) {
				start_code = 4;
			}
			else {
				continue;
			}

			if (((buffer_.get()[i + start_code] & 0x1F) == 0x5 || 
				(buffer_.get()[i + start_code] & 0x1F) == 0x1) && 
				((buffer_.get()[i + start_code + 1] & 0x80) == 0x80)) {
				is_find_start = true;
				i += 4;
				break;
			}
		}

		for (; i < bytes_read - 5; i++) {
			if (buffer_.get()[i] == 0 && buffer_.get()[i + 1] == 0 && 
				buffer_.get()[i + 2] == 1) {
				start_code = 3;
			}
			else if (buffer_.get()[i] == 0 && buffer_.get()[i + 1] == 0 && 
				buffer_.get()[i + 2] == 0 && buffer_.get()[i + 3] == 1) {
				start_code = 4;
			}
			else {
				continue;
			}

			if (((buffer_.get()[i + start_code] & 0x1F) == 0x7) || 
				((buffer_.get()[i + start_code] & 0x1F) == 0x8) || 
				((buffer_.get()[i + start_code] & 0x1F) == 0x6) || 
				(((buffer_.get()[i + start_code] & 0x1F) == 0x5 || 
				(buffer_.get()[i + start_code] & 0x1F) == 0x1) && 
				((buffer_.get()[i + start_code + 1] & 0x80) == 0x80))) {
				is_find_end = true;
				break;
			}
		}

		bool flag = false;
		if (is_find_start && !is_find_end && frame_count_ > 0) {
			flag = is_find_end = true;
			i = bytes_read;
			*end = true;
		}

		if (!is_find_start || !is_find_end) {
			this->Close();
			return -1;
		}

		int size = (i <= in_buf_size ? i : in_buf_size);
		memcpy(in_buf, buffer_.get(), size);

		if (!flag) {
			frame_count_ += 1;
			bytes_used_ += i;
		}
		else {
			frame_count_ = 0;
			bytes_used_ = 0;
		}

		fseek(file_, bytes_used_, SEEK_SET);
		return size;
	}

private:
	FILE *file_ = nullptr;
	std::shared_ptr<char> buffer_;
	int buffer_size_ = 0;
	int bytes_used_ = 0;
	int frame_count_ = 0;
	static const int kMaxFrameSize = 1024 * 200;
};

#endif