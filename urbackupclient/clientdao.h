#pragma once

#include "../Interface/Database.h"
#include "../Interface/Query.h"
#include "../urbackupcommon/os_functions.h"
#include <vector>

#ifndef GUID_DEFINED
#define GUID_DEFINED
#if defined(__midl)
typedef struct {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    byte           Data4[ 8 ];
} GUID;
#else
typedef struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;
#endif
#endif

struct SBackupDir
{
	int id;
	std::wstring tname;
	std::wstring path;
	bool optional;
	int group;
};

struct SShadowCopy
{
	SShadowCopy() : refs(0) {}
	int id;
	GUID vssid;
	GUID ssetid;
	std::wstring target;
	std::wstring path;
	std::wstring tname;
	std::wstring orig_target;
	std::wstring vol;
	std::wstring starttoken;
	bool filesrv;
	int refs;
	int passedtime;
};

struct SMDir
{
	SMDir(_i64 id, std::wstring name)
		: id(id), name(name) {}

	_i64 id;
	std::wstring name;

	bool operator<(const SMDir &other) const
	{
		return name<other.name;
	}
};

struct SFileAndHash
{
	std::wstring name;
	int64 size;
	int64 last_modified;
	bool isdir;
	std::string hash;
	std::string file_permission_bits;
	int64 last_modified_orig;
	int64 created;

	bool operator<(const SFileAndHash &other) const
	{
		return name < other.name;
	}

	bool operator==(const SFileAndHash &other) const
	{
		return name == other.name &&
			size == other.size &&
			last_modified == other.last_modified &&
			isdir == other.isdir &&
			hash == other.hash &&
			file_permission_bits == other.file_permission_bits &&
			last_modified_orig == other.last_modified_orig &&
			created == other.created;
	}
};

class ClientDAO
{
public:
	ClientDAO(IDatabase *pDB);
	void prepareQueries(void);
	void destroyQueries(void);
	void restartQueries(void);

	void prepareQueriesGen(void);
	void destroyQueriesGen(void);

	bool getFiles(std::wstring path, std::vector<SFileAndHash> &data);

	void addFiles(std::wstring path, const std::vector<SFileAndHash> &data);
	void modifyFiles(std::wstring path, const std::vector<SFileAndHash> &data);
	bool hasFiles(std::wstring path);
	
	void removeAllFiles(void);

	void copyFromTmpFiles(void);

	std::vector<SBackupDir> getBackupDirs(void);

	std::vector<SMDir> getChangedDirs(const std::wstring& path, bool del=true);

	void moveChangedFiles(_i64 dir_id, bool del=true);

	std::vector<std::wstring> getChangedFiles(_i64 dir_id);
	bool hasFileChange(_i64 dir_id, std::wstring fn);

	std::vector<SShadowCopy> getShadowcopies(void);
	int addShadowcopy(const SShadowCopy &sc);
	void deleteShadowcopy(int id);
	int modShadowcopyRefCount(int id, int m);


	void deleteSavedChangedFiles(void);
	void deleteSavedChangedDirs(void);

	bool hasChangedGap(void);
	void deleteChangedDirs(const std::wstring& path);

	std::vector<std::wstring> getGapDirs(void);

	std::vector<std::wstring> getDelDirs(const std::wstring& path, bool del=true);
	void deleteSavedDelDirs(void);

	void removeDeletedDir(const std::wstring &dir);

	std::wstring getOldExcludePattern(void);
	void updateOldExcludePattern(const std::wstring &pattern);

	std::wstring getOldIncludePattern(void);
	void updateOldIncludePattern(const std::wstring &pattern);

	std::wstring getMiscValue(const std::string& key);
	void updateMiscValue(const std::string& key, const std::wstring& value);

	//@-SQLGenFunctionsBegin
	struct CondInt64
	{
		bool exists;
		int64 value;
	};
	struct SToken
	{
		int64 id;
		std::wstring username;
		std::wstring token;
	};


	void updateShadowCopyStarttime(int id);
	void updateFileAccessToken(const std::wstring& username, const std::wstring& token);
	std::vector<SToken> getFileAccessTokens(void);
	CondInt64 getFileAccessTokenId(const std::wstring& username);
	//@-SQLGenFunctionsEnd

private:
	

	IDatabase *db;

	IQuery *q_get_files;
	IQuery *q_add_files;
	IQuery *q_get_dirs;
	IQuery *q_remove_all;
	IQuery *q_get_changed_dirs;
	IQuery *q_remove_changed_dirs;
	IQuery *q_modify_files;
	IQuery *q_has_files;
	IQuery *q_insert_shadowcopy;
	IQuery *q_get_shadowcopies;
	IQuery *q_remove_shadowcopies;
	IQuery *q_save_changed_dirs;
	IQuery *q_delete_saved_changed_dirs;
	IQuery *q_copy_from_tmp_files;
	IQuery *q_delete_tmp_files;
	IQuery *q_has_changed_gap;
	IQuery *q_get_del_dirs;
	IQuery *q_del_del_dirs;
	IQuery *q_copy_del_dirs;
	IQuery *q_del_del_dirs_copy;
	IQuery *q_remove_del_dir;
	IQuery *q_get_shadowcopy_refcount;
	IQuery *q_set_shadowcopy_refcount;
	IQuery *q_save_changed_files;
	IQuery *q_remove_changed_files;
	IQuery *q_delete_saved_changed_files;
	IQuery *q_has_changed_file;
	IQuery *q_get_changed_files;
	IQuery *q_get_pattern;
	IQuery *q_insert_pattern;
	IQuery *q_update_pattern;

	//@-SQLGenVariablesBegin
	IQuery* q_updateShadowCopyStarttime;
	IQuery* q_updateFileAccessToken;
	IQuery* q_getFileAccessTokens;
	IQuery* q_getFileAccessTokenId;
	//@-SQLGenVariablesEnd
};