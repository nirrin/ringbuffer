#include "redismodule.h"
#include "ring_buffer.h"

class RedisRingBuffer : public std::RingBuffer<RedisModuleString*> {
public:
	RedisRingBuffer(RedisModuleCtx* ctx_, const size_t size_) : std::RingBuffer<RedisModuleString * >(size_, sizeof (RedisModuleString*), false), ctx(ctx_) {
		elements = (RedisModuleString**)RedisModule_Alloc(size * an_element_size);
		for (size_t i = 0; i < size; i++) {
			elements[i] = NULL;
		}
	}

	virtual ~RedisRingBuffer() {
		for (size_t i = 0; i < size; i++) {
			if (elements[i]) {
				RedisModule_FreeString(ctx, elements[i]);
				elements[i] = NULL;
			}
		}
		RedisModule_Free(elements);
		elements = NULL;
	}

	inline size_t memory_usage() const {
		size_t size = sizeof(RedisRingBuffer);
		for (size_t i = 0; i < size; i++) {
			if (elements[i]) {
				size_t len = 0;
				RedisModule_StringPtrLen(elements[i], &len);
				size += len;
			}
		}
		return (size);
	}

	inline void write_string(const RedisModuleString* element) {
		if (elements[b_end]) {
			RedisModule_FreeString(ctx, elements[b_end]);
		}
		elements[b_end] = RedisModule_CreateStringFromString(ctx, element);
		post_write();
	}

	inline void on_load(const size_t start_, const size_t end_, const short int s_msb_, const short int e_msb_, RedisModuleString** elements_) {
		b_start = start_;
		b_end = end_;
		s_msb = s_msb_;
		e_msb = e_msb_;
		memcpy((void*)elements, (void*)elements_, size * an_element_size);
	}

	inline void on_save(size_t& start_, size_t& end_, short int& s_msb_, short int& e_msb_, RedisModuleString** elements_) {
		start_ = b_start;
		end_ = b_end;
		s_msb_ = s_msb;
		e_msb_ = e_msb;
		memcpy((void*)elements_, (void*)elements, size * an_element_size);
	}

private:
	RedisModuleCtx* ctx;
};

static RedisModuleType* RingBufferType;

void* RingBufferRdbLoad(RedisModuleIO* rdb, int encver) {
	if (encver != 0) {
		return NULL;
	}
	size_t size = (size_t)RedisModule_LoadUnsigned(rdb);
	size_t start = (size_t)RedisModule_LoadUnsigned(rdb);
	size_t end = (size_t)RedisModule_LoadUnsigned(rdb);
	short int s_msb = (short int)RedisModule_LoadSigned(rdb);
	short int e_msb = (short int)RedisModule_LoadSigned(rdb);
	RedisModuleString** elements = (RedisModuleString**)RedisModule_Alloc(sizeof(RedisModuleString*) * size);
	for (size_t i = 0; i < size; i++) {
		elements[i] = RedisModule_LoadString(rdb);
		size_t len = 0;
		RedisModule_StringPtrLen(elements[i], &len);
		if (len == 0) {
			elements[i] = NULL;
		}
	}
	RedisRingBuffer* buffer = new RedisRingBuffer(RedisModule_GetContextFromIO(rdb), size);
	buffer->on_load(start, end, s_msb, e_msb, elements);
	RedisModule_Free(elements);
	return ((void*)buffer);
}

void RingBufferRdbSave(RedisModuleIO *rdb, void* value) {
	RedisRingBuffer* buffer = (RedisRingBuffer*)value;
	size_t size = buffer->buffer_size();
	RedisModule_SaveUnsigned(rdb, size);
	size_t start = 0;
	size_t end = 0;
	short int s_msb = 0;
	short int e_msb = 0;
	RedisModuleString** elements = (RedisModuleString**)RedisModule_Alloc(sizeof(RedisModuleString*) * size);
	buffer->on_save(start, end, s_msb, e_msb, elements);
	RedisModule_SaveUnsigned(rdb, start);
	RedisModule_SaveUnsigned(rdb, end);
	RedisModule_SaveSigned(rdb, s_msb);
	RedisModule_SaveSigned(rdb, e_msb);
	for (size_t i = 0; i < size; i++) {
		if (elements[i]) {
			RedisModule_SaveString(rdb, elements[i]);
		} else {
			RedisModule_SaveString(rdb, RedisModule_CreateString(RedisModule_GetContextFromIO(rdb), "", 0));
		}
	}
	RedisModule_Free(elements);
}

void RingBufferAofRewrite(RedisModuleIO *aof, RedisModuleString *key, void *value) {
	RedisRingBuffer* buffer = (RedisRingBuffer*)value;
	RedisModule_EmitAOF(aof, "RingBufferCreate", "sl", key, buffer->buffer_size());
	while (!buffer->end()) {
		RedisModule_EmitAOF(aof, "RingBufferWrite", "ss", key, buffer->next());
	}
}

size_t RingBufferMemUsage(const void *value) {
	RedisRingBuffer* buffer = (RedisRingBuffer*)value;
	return (buffer->memory_usage());
}

void RingBufferDigest(RedisModuleDigest __attribute__((unused)) *digest, void __attribute__((unused)) *value) {
}

void RingBufferFree(void *value) {
	delete (RedisRingBuffer*)value;
}

extern "C" {
	/***
	* usage: 	RingBufferCreate name, size
	* returns: 	nil
	*/
	int RedisRingBuffer_Create_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
		RedisModule_AutoMemory(ctx);
		if (argc != 3) {
			return RedisModule_WrongArity(ctx);
		}
		RedisModuleKey* key = (RedisModuleKey*)RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
		const int type = RedisModule_KeyType(key);
		if (type != REDISMODULE_KEYTYPE_EMPTY) {
			return RedisModule_ReplyWithError(ctx, "already exist");
		}
		long long size;
		if ((RedisModule_StringToLongLong(argv[2], &size) != REDISMODULE_OK) || (size <= 0)) {
			return RedisModule_ReplyWithError(ctx, "invalid size: must be a natural number");
		}
		RedisRingBuffer* buffer = new RedisRingBuffer(ctx, (size_t)size);
		RedisModule_ModuleTypeSetValue(key, RingBufferType, buffer);
		RedisModule_ReplicateVerbatim(ctx);
		return RedisModule_ReplyWithNull(ctx);
	}

	/***
	* usage: 	RingBufferCreate name, data1 [, data2 ... ]
	* returns: 	nil
	*/
	int RedisRingBuffer_Write_RedisCommand(RedisModuleCtx *ctx, RedisModuleString** __attribute__((unused)) argv, int __attribute__((unused)) argc) {
		if (argc < 3) {
			return RedisModule_WrongArity(ctx);
		}
		RedisModuleKey* key = (RedisModuleKey*)RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
		const int type = RedisModule_KeyType(key);
		if (type == REDISMODULE_KEYTYPE_EMPTY) {
			RedisModule_CloseKey(key);
			return RedisModule_ReplyWithError(ctx, "doesn't exist");
		}
		if (RedisModule_ModuleTypeGetType(key) != RingBufferType) {
			RedisModule_CloseKey(key);
			return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
		}
		RedisRingBuffer* buffer = (RedisRingBuffer*)RedisModule_ModuleTypeGetValue(key);
		for (int i = 2; i < argc; i++) {
			buffer->write_string(argv[i]);
		}
		RedisModule_ReplicateVerbatim(ctx);
		RedisModule_CloseKey(key);
		return RedisModule_ReplyWithNull(ctx);
	}

#define RING_BUFFER 			RedisModule_AutoMemory(ctx); \
								if (argc != 2) { \
									return RedisModule_WrongArity(ctx); \
								} \
								RedisModuleKey* key = (RedisModuleKey*)RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE); \
								const int type = RedisModule_KeyType(key); \
								if (type == REDISMODULE_KEYTYPE_EMPTY) { \
									return RedisModule_ReplyWithError(ctx, "doesn't exist"); \
								} \
								if (RedisModule_ModuleTypeGetType(key) != RingBufferType) { \
									return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE); \
								} \
								RedisRingBuffer* buffer = (RedisRingBuffer*)RedisModule_ModuleTypeGetValue(key);

	/***
	* usage: 	RingBufferRead name
	* returns: 	if the ring buffer is empty nill otherwise the first value
	*/
	int RedisRingBuffer_Read_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
		RING_BUFFER
		if (buffer->is_empty()) {
			return RedisModule_ReplyWithNull(ctx);
		} else {
			RedisModuleString* value = buffer->read();
			RedisModule_ReplicateVerbatim(ctx);
			return RedisModule_ReplyWithString(ctx, value);
		}
	}

	/***
	* usage: 	RingBufferLength name
	* returns: 	the length of the ring buffer
	*/
	int RedisRingBuffer_Length_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
		RING_BUFFER
		return RedisModule_ReplyWithLongLong(ctx, buffer->length());
	}

	/***
	* usage: 	RingBufferIsFull name
	* returns: 	1 if the buffer is full, 0 otherwise
	*/
	int RedisRingBuffer_IsFull_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
		RING_BUFFER
		return RedisModule_ReplyWithLongLong(ctx, buffer->is_full());
	}

	/***
	* usage: 	RingBufferIsEmpty name
	* returns: 	1 if the buffer is full, 0 otherwise
	*/
	int RedisRingBuffer_IsEmpty_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
		RING_BUFFER
		return RedisModule_ReplyWithLongLong(ctx, buffer->is_empty());
	}

	/***
	* usage: 	RingBufferSize name
	* returns: 	the ring buffer size
	*/
	int RedisRingBuffer_Size_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
		RING_BUFFER
		return RedisModule_ReplyWithLongLong(ctx, buffer->buffer_size());
	}

	/***
	* usage: 	RingBufferFront name
	* returns: 	if the ring buffer is empty nil, otherwise the front of the buffer
	*/
	int RedisRingBuffer_Front_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
		RING_BUFFER
		if (buffer->is_empty()) {
			return RedisModule_ReplyWithNull(ctx);
		} else {
			return RedisModule_ReplyWithString(ctx, buffer->front());
		}
	}

	/***
	* usage: 	RingBufferBack name
	* returns: 	if the ring buffer is empty nil, otherwise the back of the buffer
	*/
	int RedisRingBuffer_Back_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
		RING_BUFFER
		if (buffer->is_empty()) {
			return RedisModule_ReplyWithNull(ctx);
		} else {
			return RedisModule_ReplyWithString(ctx, buffer->back());
		}
	}

	/***
	* usage: 	RingBufferReadAll name
	* returns: 	if the ring buffer is empty nil, otherwise a list of the buffer values
	*/
	int RedisRingBuffer_ReadAll_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
		RING_BUFFER
		if (buffer->is_empty()) {
			return RedisModule_ReplyWithNull(ctx);
		} else {
			RedisModule_ReplyWithArray(ctx, REDISMODULE_POSTPONED_ARRAY_LEN);
			buffer->begin();
			size_t length = 0;
			while (!buffer->end()) {
				RedisModule_ReplyWithString(ctx, buffer->next());
				length++;
			}
			RedisModule_ReplySetArrayLength(ctx, length);
			RedisModule_ReplicateVerbatim(ctx);
			return REDISMODULE_OK;
		}
	}

	/***
	* usage: 	RingBufferClear name
	* returns: 	nil
	*/
	int RedisRingBuffer_Clear_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
		RING_BUFFER
		buffer->clear();
		return RedisModule_ReplyWithNull(ctx);
	}

#define CREATE_COMMAND(name, command, policy)	if (RedisModule_CreateCommand(ctx, name, command, policy, 1, 1, 1) == REDISMODULE_ERR) { \
													return REDISMODULE_ERR; \
												}

	int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString __attribute__((unused)) **argv, int __attribute__((unused)) argc) {
		if (RedisModule_Init(ctx, "ringbuffer", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
			return REDISMODULE_ERR;
		}
		RedisModuleTypeMethods tm = {
			.version = REDISMODULE_TYPE_METHOD_VERSION,
			.rdb_load = RingBufferRdbLoad,
			.rdb_save = RingBufferRdbSave,
			.aof_rewrite = RingBufferAofRewrite,
			.mem_usage = RingBufferMemUsage,
			.digest = RingBufferDigest,
			.free = RingBufferFree
		};
		RingBufferType = RedisModule_CreateDataType(ctx, "ringbuffr", 0, &tm);
		if (RingBufferType == NULL) {
			return REDISMODULE_ERR;
		}
		CREATE_COMMAND("RingBufferCreate", RedisRingBuffer_Create_RedisCommand, "write deny-oom");
		CREATE_COMMAND("RingBufferWrite", RedisRingBuffer_Write_RedisCommand, "write deny-oom");
		CREATE_COMMAND("RingBufferRead", RedisRingBuffer_Read_RedisCommand, "write");
		CREATE_COMMAND("RingBufferLength", RedisRingBuffer_Length_RedisCommand, "readonly");
		CREATE_COMMAND("RingBufferIsFull", RedisRingBuffer_IsFull_RedisCommand, "readonly");
		CREATE_COMMAND("RingBufferIsEmpty", RedisRingBuffer_IsEmpty_RedisCommand, "readonly");
		CREATE_COMMAND("RingBufferSize", RedisRingBuffer_Size_RedisCommand, "readonly");
		CREATE_COMMAND("RingBufferFront", RedisRingBuffer_Front_RedisCommand, "readonly");
		CREATE_COMMAND("RingBufferBack", RedisRingBuffer_Back_RedisCommand, "readonly");
		CREATE_COMMAND("RingBufferReadAll", RedisRingBuffer_ReadAll_RedisCommand, "write");
		CREATE_COMMAND("RingBufferClear", RedisRingBuffer_Clear_RedisCommand, "write");
		return REDISMODULE_OK;
	}
}