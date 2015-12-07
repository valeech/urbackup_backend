#include "../tclap/CmdLine.h"
#include "../tclap/DocBookOutput.h"
#include <vector>
#include "../stringtools.h"
#include "../urbackupcommon/os_functions.h"

#ifndef _WIN32
#	include <sys/types.h>
#	include <pwd.h>
#	include <sys/wait.h>
#	include <ctime>
#	include <sys/time.h>
#	include <unistd.h>
#	include <sys/types.h>
#	include <pwd.h>
#	include <sys/stat.h>
#	include <sys/types.h>
#	include <sys/fcntl.h>
#endif

const std::string cmdline_version = "2.0";

void show_version()
{
	std::cout << "UrBackup Server v" << cmdline_version << std::endl;
	std::cout << "Copyright (C) 2011-2015 Martin Raiber" << std::endl;
	std::cout << "This is free software; see the source for copying conditions. There is NO"<< std::endl;
	std::cout << "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."<< std::endl;
}

int64 get_time_ms()
{
#ifndef _WIN32
	timespec tp;
	if(clock_gettime(CLOCK_MONOTONIC, &tp)!=0)
	{
		timeval tv;
		gettimeofday(&tv, NULL);
		static long start_t=tv.tv_sec;
		tv.tv_sec-=start_t;
		return tv.tv_sec*1000+tv.tv_usec/1000;
	}
	return static_cast<int64>(tp.tv_sec)*1000+tp.tv_nsec/1000000;
#else
	return 0;
#endif
}

int real_main(int argc, char* argv[]);

int run_real_main(std::vector<std::string> args)
{
	char** argv = new char*[args.size()];
	for(size_t i=0;i<args.size();++i)
	{
		argv[i] = const_cast<char*>(args[i].c_str());
	}

	int rc = real_main(static_cast<int>(args.size()), argv);
	delete[] argv;
	return rc;
}

typedef int(*action_fun)(std::vector<std::string> args);

int action_run(std::vector<std::string> args)
{
	TCLAP::CmdLine cmd("Run network file and image backup server", ' ', cmdline_version);

	TCLAP::ValueArg<unsigned short> fastcgi_port_arg("f", "fastcgi-port",
		"Specifies the port where UrBackup server will listen for FastCGI connections",
		false, 55413, "port number", cmd);

	TCLAP::ValueArg<unsigned short> http_port_arg("p", "http-port",
		"Specifies the port where UrBackup server will listen for HTTP connections",
		false, 55414, "port number", cmd);

	TCLAP::ValueArg<std::string> logfile_arg("l", "logfile",
		"Specifies the log file name",
		false, "/var/log/urbackup.log", "path", cmd);

	std::vector<std::string> loglevels;
	loglevels.push_back("debug");
	loglevels.push_back("info");
	loglevels.push_back("warn");
	loglevels.push_back("error");

	TCLAP::ValuesConstraint<std::string> loglevels_constraint(loglevels);
	TCLAP::ValueArg<std::string> loglevel_arg("v", "loglevel",
		"Specifies the log level",
		false, "info", &loglevels_constraint, cmd);

	TCLAP::SwitchArg no_console_time_arg("t", "no-consoletime",
		"Do not print time when logging to console",
		cmd, false);

	TCLAP::SwitchArg daemon_arg("d", "daemon", "Daemonize process", cmd, false);

	TCLAP::ValueArg<std::string> pidfile_arg("w", "pidfile",
		"Save pid of daemon in file",
		false, "/var/run/urbackup_srv.pid", "path", cmd);

	TCLAP::MultiArg<std::string> broadcast_interface_arg("b", "broadcast-interface",
		"Network interface from which to send broadcasts", false, "network interface name", cmd);

	TCLAP::ValueArg<std::string> sqlite_tmpdir_arg("s", "sqlite-tmpdir",
		"Specifies the directory SQLite uses to store temporary tables",
		false, "", "path", cmd);

	TCLAP::ValueArg<std::string> user_arg("u", "user",
		"Change process to run as specific user",
		false, "urbackup", "user", cmd);

	cmd.parse(args);

	std::vector<std::string> real_args;
	real_args.push_back(args[0]);
	real_args.push_back("--port");
	real_args.push_back(nconvert(fastcgi_port_arg.getValue()));
	real_args.push_back("--http_port");
	real_args.push_back(nconvert(http_port_arg.getValue()));
	real_args.push_back("--pidfile");
	real_args.push_back(pidfile_arg.getValue());
	real_args.push_back("--user");
	real_args.push_back(user_arg.getValue());
	if(!sqlite_tmpdir_arg.getValue().empty())
	{
		real_args.push_back("--sqlite_tmpdir");
		real_args.push_back(sqlite_tmpdir_arg.getValue());
	}

	std::string broadcast_interfaces;
	const std::vector<std::string>& bi = broadcast_interface_arg.getValue();
	for(size_t i=0;i<bi.size();++i)
	{
		if(!broadcast_interfaces.empty())
		{
			broadcast_interfaces+=",";
		}
		broadcast_interfaces+=bi[i];
	}
	if(!broadcast_interfaces.empty())
	{
		real_args.push_back("--broadcast_interfaces");
		real_args.push_back(broadcast_interfaces);
	}
	real_args.push_back("--logfile");
	real_args.push_back(logfile_arg.getValue());
	real_args.push_back("--loglevel");
	real_args.push_back(loglevel_arg.getValue());
	if(daemon_arg.getValue())
	{
		real_args.push_back("--daemon");
	}
	if(no_console_time_arg.getValue())
	{
		real_args.push_back("--log_console_no_time");
	}

	return run_real_main(real_args);
}

int action_verify_hashes(std::vector<std::string> args)
{
	TCLAP::CmdLine cmd("Verify file backup hashes", ' ', cmdline_version);

	TCLAP::SwitchArg delete_verify_failed_arg("d", "delete-verify-failed",
		"Delete file entries of files with failed verification", cmd, false);

	TCLAP::ValueArg<std::string> verify_arg("v", "verify",
		"Specify file backup(s) to verify",
		true, "all", "file backup set", cmd);

	TCLAP::ValueArg<std::string> user_arg("u", "user",
		"Change process to run as specific user",
		false, "urbackup", "user", cmd);

	cmd.parse(args);

	std::vector<std::string> real_args;
	real_args.push_back(args[0]);
	real_args.push_back("--no-server");
	real_args.push_back("--user");
	real_args.push_back(user_arg.getValue());
	real_args.push_back("--loglevel");
	real_args.push_back("debug");
	if(delete_verify_failed_arg.getValue())
	{
		real_args.push_back("--delete_verify_failed");
		real_args.push_back("true");
	}

	if(verify_arg.getValue()=="all")
	{
		real_args.push_back("--verify_hashes");
		real_args.push_back("true");
	}
	else
	{
		real_args.push_back("--verify_hashes");
		real_args.push_back(verify_arg.getValue());
	}

	return run_real_main(real_args);
}

int action_remove_unknown(std::vector<std::string> args)
{
	TCLAP::CmdLine cmd("Remove unknown files and directories from backup storage and fix symbolic links in backup storage", ' ', cmdline_version);

	TCLAP::ValueArg<std::string> user_arg("u", "user",
		"Change process to run as specific user",
		false, "urbackup", "user", cmd);

	cmd.parse(args);

	std::vector<std::string> real_args;
	real_args.push_back(args[0]);
	real_args.push_back("--no-server");
	real_args.push_back("--user");
	real_args.push_back(user_arg.getValue());
	real_args.push_back("--loglevel");
	real_args.push_back("debug");
	real_args.push_back("--app");
	real_args.push_back("remove_unknown");

	return run_real_main(real_args);
}

int action_defrag_database(std::vector<std::string> args)
{
	TCLAP::CmdLine cmd("Rebuild UrBackup database", ' ', cmdline_version);

	TCLAP::ValueArg<std::string> user_arg("u", "user",
		"Change process to run as specific user",
		false, "urbackup", "user", cmd);

	cmd.parse(args);

	std::vector<std::string> real_args;
	real_args.push_back(args[0]);
	real_args.push_back("--no-server");
	real_args.push_back("--user");
	real_args.push_back(user_arg.getValue());
	real_args.push_back("--loglevel");
	real_args.push_back("debug");
	real_args.push_back("--app");
	real_args.push_back("defrag_database");

	return run_real_main(real_args);
}

int action_repair_database(std::vector<std::string> args)
{
	TCLAP::CmdLine cmd("Try to repair UrBackup database", ' ', cmdline_version);

	TCLAP::ValueArg<std::string> user_arg("u", "user",
		"Change process to run as specific user",
		false, "urbackup", "user", cmd);

	cmd.parse(args);

	std::vector<std::string> real_args;
	real_args.push_back(args[0]);
	real_args.push_back("--no-server");
	real_args.push_back("--user");
	real_args.push_back(user_arg.getValue());
	real_args.push_back("--loglevel");
	real_args.push_back("debug");
	real_args.push_back("--app");
	real_args.push_back("repair_database");

	return run_real_main(real_args);
}

int action_reset_pw(std::vector<std::string> args)
{
	TCLAP::CmdLine cmd("Reset web interface administrator password", ' ', cmdline_version);

	TCLAP::ValueArg<std::string> password_arg("p", "password",
		"New administrator password",
		true, "", "password", cmd);

	TCLAP::ValueArg<std::string> user_arg("u", "user",
		"Change process to run as specific user",
		false, "urbackup", "user", cmd);

	cmd.parse(args);

	std::vector<std::string> real_args;
	real_args.push_back(args[0]);
	real_args.push_back("--no-server");
	real_args.push_back("--user");
	real_args.push_back(user_arg.getValue());
	real_args.push_back("--loglevel");
	real_args.push_back("debug");
	real_args.push_back("--set_admin_pw");
	real_args.push_back(password_arg.getValue());

	return run_real_main(real_args);
}

int action_cleanup(std::vector<std::string> args)
{
	TCLAP::CmdLine cmd("Cleanup file/image backups from backup storage", ' ', cmdline_version);

	TCLAP::ValueArg<std::string> cleanup_amount_arg("a", "amount",
		"Amount of storage to cleanup",
		true, "", "%|M|G|T", cmd);

	TCLAP::ValueArg<std::string> user_arg("u", "user",
		"Change process to run as specific user",
		false, "urbackup", "user", cmd);

	cmd.parse(args);

	std::vector<std::string> real_args;
	real_args.push_back(args[0]);
	real_args.push_back("--no-server");
	real_args.push_back("--user");
	real_args.push_back(user_arg.getValue());
	real_args.push_back("--loglevel");
	real_args.push_back("debug");
	real_args.push_back("--app");
	real_args.push_back("cleanup");
	real_args.push_back("--cleanup_amount");
	real_args.push_back(cleanup_amount_arg.getValue());

	return run_real_main(real_args);
}

int action_export_auth_log(std::vector<std::string> args)
{
	TCLAP::CmdLine cmd("Export authentication log to csv file", ' ', cmdline_version);

	cmd.parse(args);

	std::vector<std::string> real_args;
	real_args.push_back(args[0]);
	real_args.push_back("--no-server");
	real_args.push_back("--loglevel");
	real_args.push_back("debug");
	real_args.push_back("--app");
	real_args.push_back("export_auth_log");

	return run_real_main(real_args);
}

int action_decompress_file(std::vector<std::string> args)
{
	TCLAP::CmdLine cmd("Decompress UrBackup compressed file", ' ', cmdline_version);

	TCLAP::ValueArg<std::string> file_arg("f", "file",
		"File to decompress",
		true, "", "path", cmd);

	TCLAP::ValueArg<std::string> user_arg("u", "user",
		"Change process to run as specific user",
		false, "urbackup", "user", cmd);

	cmd.parse(args);

	std::vector<std::string> real_args;
	real_args.push_back(args[0]);
	real_args.push_back("--no-server");
	real_args.push_back("--user");
	real_args.push_back(user_arg.getValue());
	real_args.push_back("--loglevel");
	real_args.push_back("debug");
	real_args.push_back("--decompress");
	real_args.push_back(file_arg.getValue());

	return run_real_main(real_args);
}

#ifndef _WIN32
int action_mount_vhd(std::vector<std::string> args)
{
	TCLAP::CmdLine cmd("Mount VHD file", ' ', cmdline_version);

	TCLAP::ValueArg<std::string> file_arg("f", "file",
		"VHD(z) file to mount",
		true, "", "path", cmd);

	TCLAP::ValueArg<std::string> mountpoint_arg("m", "mountpoint",
		"Mount the VHD(z)-file at this mountpoint",
		true, "", "path", cmd);

	TCLAP::ValueArg<std::string> tempmount_arg("t", "tempmount",
		"Use this directory as temporary mountpoint",
		false, "", "path", cmd);

	TCLAP::ValueArg<std::string> logfile_arg("l", "logfile",
		"Specifies the log file name for the background VHD-reading process",
		false, "/var/log/urbackup-fuse.log", "path", cmd);

	cmd.parse(args);

	std::vector<std::string> real_args;
	real_args.push_back(args[0]);
	real_args.push_back("--no-server");
	real_args.push_back("--logfile");
	real_args.push_back(logfile_arg.getValue());
	real_args.push_back("--loglevel");
	real_args.push_back("debug");
	real_args.push_back("--mount");
	real_args.push_back(file_arg.getValue());

	std::string tmpmountpoint;

	if(tempmount_arg.getValue().empty())
	{
		char *tmpdir=getenv("TMPDIR");
		std::string stmpdir;
		if(tmpdir==NULL )
			stmpdir="/tmp";
		else
			stmpdir=tmpdir; 

		stmpdir=stmpdir+"/cps.XXXXXX";

		char* made_tmpdir = mkdtemp(&stmpdir[0]);
		if(made_tmpdir==NULL)
		{
			std::cout << "Error creating temporary directory " << stmpdir << std::endl;
			exit(1);
		}

		tmpmountpoint = stmpdir;		
	}
	else
	{
		tmpmountpoint = tempmount_arg.getValue();
	}

	real_args.push_back("--mountpoint");
	real_args.push_back(tmpmountpoint);

	std::cout << "Loading FUSE kernel module..." << std::endl;
	system("modprobe fuse");

	std::cout << "Starting VHD background process..." << std::endl;
	size_t pid1;
	if( (pid1=fork())==0 )
	{
		if(setsid()==-1)
		{
			perror("urbackupsrv");
			exit(1);
		}
		if(fork()==0)
		{
			for (int i=getdtablesize();i>=0;--i) close(i);
			int i=open("/dev/null",O_RDWR);
			dup(i);
			dup(i);
			return run_real_main(real_args);
		}
		else
		{
			exit(0);
		}
	}
	else
	{
		int status;
		int rc = waitpid(pid1, &status, 0);
		if(rc==-1)
		{
			perror("urbackupsrv");
			exit(1);
		}
	}

	std::cout << "Waiting for background process to become available..." << std::endl;
	int64 starttime = get_time_ms();
	while(get_time_ms()-starttime<20000)
	{
		if(FileExists(tmpmountpoint+"/volume"))
		{
			std::cout << "Mounting..." << std::endl;
			int rc = system(("mount -v -o loop,ro \""+tmpmountpoint+"/volume\" \""+mountpoint_arg.getValue()).c_str());
			if(rc!=0)
			{
				std::cout << "Mounting failed." << std::endl;
				exit(1);
			}
			else
			{
				std::cout << "Mounted successfully." << std::endl;
				exit(0);
			}
		}
		usleep(100*1000);
	}

	std::cout << "Timeout while waiting for background process. Please see logfile (" << logfile_arg.getValue() << ") for error details." << std::endl;
	exit(1);
}
#endif

int action_assemble(std::vector<std::string> args)
{
	TCLAP::CmdLine cmd("Assemble VHD(Z) volumes into one disk VHD file", ' ', cmdline_version);

	TCLAP::MultiArg<std::string> assemble_in_arg("a", "assemble-in",
		"File to assemble into the output file",
		true, "path", cmd);

	TCLAP::ValueArg<std::string> assemble_out("o", "assemble-out",
		"Location where disk VHD is written when assembling",
		true, "", "path", cmd);

	cmd.parse(args);

	std::string assemble_input;
	for(size_t i=0;i<assemble_in_arg.getValue().size();++i)
	{
		if(!assemble_input.empty())
		{
			assemble_input+=";";
		}
		assemble_input+=assemble_in_arg.getValue()[i];
	}

	std::vector<std::string> real_args;
	real_args.push_back(args[0]);
	real_args.push_back("--no-server");
	real_args.push_back("--loglevel");
	real_args.push_back("debug");
	real_args.push_back("--assemble");
	real_args.push_back(assemble_input);
	real_args.push_back("--output_file");
	real_args.push_back(assemble_out.getValue());

	return run_real_main(real_args);
}

void action_help(std::string cmd)
{
	std::cout << std::endl;
	std::cout << "USAGE:" << std::endl;
	std::cout << std::endl;
	std::cout << "\t" << cmd << " [--help] [--version] <command> [<args>]" << std::endl;
	std::cout << std::endl;
	std::cout << "Get specific command help with " << cmd << " <command> --help" << std::endl;
	std::cout << std::endl;
	std::cout << "\t" << cmd << " run" << std::endl;
	std::cout << "\t\t" "Run UrBackup server" << std::endl;
	std::cout << std::endl;
	std::cout << "\t" << cmd << " verify-hashes" << std::endl;
	std::cout << "\t\t" "Verify file backup hashes" << std::endl;
	std::cout << std::endl;
	std::cout << "\t" << cmd << " remove-unknown" << std::endl;
	std::cout << "\t\t" "Remove unknown files and directories from backup storage and fix symbolic links in backup storage" << std::endl;
	std::cout << std::endl;
	std::cout << "\t" << cmd << " reset-pw" << std::endl;
	std::cout << "\t\t" "Reset web interface administrator password" << std::endl;
	std::cout << std::endl;
	std::cout << "\t" << cmd << " cleanup" << std::endl;
	std::cout << "\t\t" "Cleanup file/image backups from backup storage" << std::endl;
	std::cout << std::endl;
	std::cout << "\t" << cmd << " repair-database" << std::endl;
	std::cout << "\t\t" "Try to repair UrBackup database" << std::endl;
	std::cout << std::endl;
	std::cout << "\t" << cmd << " defrag-database" << std::endl;
	std::cout << "\t\t" "Rebuild UrBackup database" << std::endl;
	std::cout << std::endl;
	std::cout << "\t" << cmd << " export-auth-log" << std::endl;
	std::cout << "\t\t" "Export authentication log to csv file" << std::endl;
	std::cout << std::endl;
	std::cout << "\t" << cmd << " decompress-file" << std::endl;
	std::cout << "\t\t" "Decompress UrBackup compressed file" << std::endl;
	std::cout << std::endl;
#if !defined(_WIN32) && defined(WITH_FUSEPLUGIN)
	std::cout << "\t" << cmd << " mount-vhd" << std::endl;
	std::cout << "\t\t" "Mount VHD file" << std::endl;
	std::cout << std::endl;
#endif
	std::cout << "\t" "urbackupsrv assemble" << std::endl;
	std::cout << "\t\t" "Assemble VHD(Z) volumes into one disk VHD file" << std::endl;
	std::cout << std::endl;
}

int main(int argc, char* argv[])
{
	if(argc==0)
	{
		std::cout << "Not enough arguments (zero arguments) -- no program name" << std::endl;
		return 1;
	}

	std::vector<std::string> actions;
	std::vector<action_fun> action_funs;
	actions.push_back("run");
	action_funs.push_back(action_run);
	actions.push_back("verify-hashes");
	action_funs.push_back(action_verify_hashes);
	actions.push_back("remove-unknown");
	action_funs.push_back(action_remove_unknown);
	actions.push_back("reset-pw");
	action_funs.push_back(action_reset_pw);
	actions.push_back("cleanup");
	action_funs.push_back(action_cleanup);
	actions.push_back("repair-database");
	action_funs.push_back(action_repair_database);
	actions.push_back("defrag-database");
	action_funs.push_back(action_defrag_database);
	actions.push_back("export-auth-log");
	action_funs.push_back(action_export_auth_log);
	actions.push_back("decompress-file");
	action_funs.push_back(action_decompress_file);
#if !defined(_WIN32) && defined(WITH_FUSEPLUGIN)
	actions.push_back("mount-vhd");
	action_funs.push_back(action_mount_vhd);
#endif
	actions.push_back("assemble");
	action_funs.push_back(action_assemble);

	bool has_help=false;
	bool has_version=false;
	size_t action_idx=std::string::npos;
	std::vector<std::string> args;
	args.push_back(argv[0]);
	for(size_t i=1;i<argc;++i)
	{
		std::string arg = argv[i];

		if(arg=="--help" || arg=="-h")
		{
			has_help=true;
		}

		if(arg=="--version" || arg=="-v")
		{
			has_version=true;
		}

		if(!arg.empty() && arg[0]=='-')
		{
			args.push_back(arg);
			continue;
		}
		
		bool found_action=false;
		for(size_t j=0;j<actions.size();++j)
		{
			if(next(actions[j], 0, arg))
			{
				if(action_idx!=std::string::npos)
				{
					action_help(argv[0]);
					exit(1);
				}
				action_idx=j;
				found_action=true;
			}
		}

		if(!found_action)
		{
			args.push_back(arg);
		}
	}

	if(action_idx==std::string::npos)
	{
		if(has_help)
		{
			action_help(argv[0]);
			exit(1);
		}
		if(has_version)
		{
			show_version();
			exit(1);
		}
		action_help(argv[0]);
		exit(1);
	}

	try
	{
		args[0]+=" "+actions[action_idx];
		int rc = action_funs[action_idx](args);
		return rc;
	}
	catch (TCLAP::ArgException &e)
	{
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
		return 1;
	}
}