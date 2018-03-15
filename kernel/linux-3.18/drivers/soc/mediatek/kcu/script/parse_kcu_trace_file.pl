#!/user/bin/perl

use strict;
use warnings;
#the key signal no.
use constant EXP_SIG_NO =>35;

#syscall .no define
use constant SYS_RT_SIGRETURN_WRAPPER =>173;
use constant SYS_RT_SIGTIMEDWAIT =>177;
use constant SYS_RT_SIGSUSPEND =>179;
use constant KEY_SYS_RT_SIGRETURN_WRAPPER =>"sys_rt_sig_ret";
use constant KEY_SYS_RT_SIGTIMEDWAIT =>"sys_rt_sigwait";
use constant KEY_SYS_RT_SIGSUSPEND =>"sys_rt_sigsuspend";
use constant KEY_SYS_ENTER =>"enter_in_";
use constant KEY_SYS_EXIT =>"ret_from_";
#keyword in trace file
use constant SIG_GENERATE =>"signal_generate";
use constant SIG_DELIVER =>"signal_deliver";
use constant SYSCALL_ENTER =>"sys_enter";
use constant SYSCALL_EXIT =>"sys_exit";
#stage
use constant STAGE_SIG_GENERATE =>1;
use constant STAGE_SIG_DELIVER =>2;
use constant STAGE_ENTER_RT_SIGRET =>3;
use constant STAGE_EXIT_RT_SIGRET =>4;
use constant STAGE_ENTER_RT_SIGWAIT =>5;
use constant STAGE_EXIT_RT_SIGWAIT =>6;
use constant STAGE_ENTER_RT_SIGSUSP =>7;
use constant STAGE_EXIT_RT_SIGSUSP =>8;

#filename
use constant SYNC_SIGNAL_VS_TIME => "sync_sig_vs_time.txt";
use constant ASYNC_SIGNAL_VS_TIME => "async_sig_vs_time.txt";
use constant ALL_TASK_VS_TIME => "task_vs_time.txt";

use constant SYNC_THD0_NAME => "sync_kcu_\/0";
use constant SYNC_THD1_NAME => "sync_kcu_\/1";
use constant SYNC_THD2_NAME => "sync_kcu_\/2";
use constant SYNC_THD3_NAME => "sync_kcu_\/3";
use constant ASYNC_THD0_NAME => "async_kcu\/0";
use constant ASYNC_THD1_NAME => "async_kcu\/1";


#timestamp muxer
use constant TIME_STAMP_MUXER => 1e6;

#store the signal generate time,signal index
my @sync_thd0_gen;
my @sync_thd1_gen;
my @sync_thd2_gen;
my @sync_thd3_gen;
my @async_thd0_gen;
my @async_thd1_gen;
#store the signal index and signal generate time by task
my %sync_thd0_hash;
my %sync_thd1_hash;
my %sync_thd2_hash;
my %sync_thd3_hash;
my %async_thd0_hash;
my %async_thd1_hash;
my %sync_thd0_sig_idx_hash;
my %sync_thd1_sig_idx_hash;
my %sync_thd2_sig_idx_hash;
my %sync_thd3_sig_idx_hash;
my %async_thd0_sig_idx_hash;
my %async_thd1_sig_idx_hash;

my %task_lst_stage_hash;

sub is_sync_task
{
	my $task=shift;
	
	if($task=~m/async/)
	{
		return 0;
	}
	return 1;
}
sub get_task_name
{
	my $line=shift;
	
	if($line=~m/comm=(.+?)\s/)
	{
		return $1;
	}
	return "";
}
sub get_curr_task_name
{
	my $line=shift;
	
	if($line=~m/^\s+(.*?)-\d/)
	{
		return $1;
	}
	return "";
}

sub get_timestamp
{
	my $line=shift;
	my @wordlist1=split ":",$line;
	my @wordlist2=split " ",$wordlist1[0];
	my $word2cnt=@wordlist2;
	
	return $wordlist2[$word2cnt-1];
}
sub get_sig_info_by_task
{
	my $task=shift;
	my $key=shift;
	my $start_time;
	my $sigindex;
	my @result;
	
	#if($task eq "sync_kcu_\/0")
	if($task eq (SYNC_THD0_NAME))
	{
		$start_time=$sync_thd0_hash{$key};
		$sigindex=$sync_thd0_sig_idx_hash{$key};
	}
	#if($task eq "sync_kcu_\/1")
	if($task eq (SYNC_THD1_NAME))	
	{
		$start_time=$sync_thd1_hash{$key};
		$sigindex=$sync_thd1_sig_idx_hash{$key};			
	}
	#if($task eq "sync_kcu_\/2")
	if($task eq (SYNC_THD2_NAME))
	{
		$start_time=$sync_thd2_hash{$key};	
		$sigindex=$sync_thd2_sig_idx_hash{$key};
	}
	#if($task eq "sync_kcu_\/3")
	if($task eq (SYNC_THD3_NAME))
	{
		$start_time=$sync_thd3_hash{$key};
		$sigindex=$sync_thd3_sig_idx_hash{$key};
	}
	#if($task eq "async_kcu\/0")
	if($task eq (ASYNC_THD0_NAME))	
	{
		$start_time=$async_thd0_hash{$key};
		$sigindex=$async_thd0_sig_idx_hash{$key};
	}
	#if($task eq "async_kcu\/1")
	if($task eq (ASYNC_THD1_NAME))	
	{
		$start_time=$async_thd1_hash{$key};
		$sigindex=$async_thd1_sig_idx_hash{$key};			
	}
	push @result,$start_time;
	push @result,$sigindex;	
	return @result;
}

sub save_sig_info_by_task
{
	my $task=shift;
	my $time=shift;
	my $sigindex=shift;
	my $key=shift;
	
	if($task eq (SYNC_THD0_NAME))
	{
		$sync_thd0_hash{$key}=$time;
		$sync_thd0_sig_idx_hash{$key}=$sigindex;
	}
	if($task eq (SYNC_THD1_NAME))
	{
		$sync_thd1_hash{$key}=$time;
		$sync_thd1_sig_idx_hash{$key}=$sigindex;			
	}
	if($task eq (SYNC_THD2_NAME))
	{
		$sync_thd2_hash{$key}=$time;
		$sync_thd2_sig_idx_hash{$key}=$sigindex;		
	}
	if($task eq (SYNC_THD3_NAME))
	{
		$sync_thd3_hash{$key}=$time;
		$sync_thd3_sig_idx_hash{$key}=$sigindex;
	}
	if($task eq (ASYNC_THD0_NAME))
	{
		$async_thd0_hash{$key}=$time;
		$async_thd0_sig_idx_hash{$key}=$sigindex;
	}
	if($task eq (ASYNC_THD1_NAME))
	{
		$async_thd1_hash{$key}=$time;
		$async_thd1_sig_idx_hash{$key}=$sigindex;			
	}	
}

sub save_signal_generate_info
{
	my $task=shift;
	my $signal_index=shift;
	my $time=shift;	
		
		if($task eq (SYNC_THD0_NAME))
		{
			push @sync_thd0_gen,$time;
			push @sync_thd0_gen,$signal_index;			
			#print "IS sync_kcu_/0";
		}
		if($task eq (SYNC_THD1_NAME))
		{
			push @sync_thd1_gen,$time;
			push @sync_thd1_gen,$signal_index;			
		}
		if($task eq (SYNC_THD2_NAME))
		{
			push @sync_thd2_gen,$time;
			push @sync_thd2_gen,$signal_index;		
		}
		if($task eq (SYNC_THD3_NAME))
		{
			push @sync_thd3_gen,$time;
			push @sync_thd3_gen,$signal_index;	
		}
		if($task eq (ASYNC_THD0_NAME))
		{
			push @async_thd0_gen,$time;
			push @async_thd0_gen,$signal_index;		
		}		
		if($task eq (ASYNC_THD1_NAME))
		{
			push @async_thd1_gen,$time;
			push @async_thd1_gen,$signal_index;			
		}
}
sub get_signal_generate_info
{
	my $task=shift;
	my $remove=shift;
	my @removed;
		
		if($task eq (SYNC_THD0_NAME))
		{
			if($remove)
			{
				@removed=splice @sync_thd0_gen,0,2; 
			}
			else
			{
				push @removed,$sync_thd0_gen[0];
				push @removed,$sync_thd0_gen[1];
			}
		}
		if($task eq (SYNC_THD1_NAME))
		{
			if($remove)
			{
				@removed=splice @sync_thd1_gen,0,2; 
			}
			else
			{
				push @removed,$sync_thd1_gen[0];
				push @removed,$sync_thd1_gen[1];
			}		
		}
		if($task eq (SYNC_THD2_NAME))
		{
			if($remove)
			{
				@removed=splice @sync_thd2_gen,0,2; 
			}
			else
			{
				push @removed,$sync_thd2_gen[0];
				push @removed,$sync_thd2_gen[1];
			}	
		}
		if($task eq (SYNC_THD3_NAME))
		{
			if($remove)
			{
				@removed=splice @sync_thd3_gen,0,2; 
			}
			else
			{
				push @removed,$sync_thd3_gen[0];
				push @removed,$sync_thd3_gen[1];
			}
		}
		if($task eq (ASYNC_THD0_NAME))
		{
			if($remove)
			{
				@removed=splice @async_thd0_gen,0,2; 
			}
			else
			{
				push @removed,$async_thd0_gen[0];
				push @removed,$async_thd0_gen[1];
			}
		}		
		if($task eq (ASYNC_THD1_NAME))
		{
			if($remove)
			{
				@removed=splice @async_thd1_gen,0,2; 
			}
			else
			{
				push @removed,$async_thd1_gen[0];
				push @removed,$async_thd1_gen[1];
			}
		}
	return @removed;	
}
sub get_syscall_no
{
	my $line=shift;
	
	my @wordlist1=split ":",$line;
	my @wordlist2=split " ",$wordlist1[2];
	if($wordlist2[0] eq "NR")
	{
		return $wordlist2[1];
	}
	return undef;
}
sub get_syscall_ret
{
	my $line=shift;
	
	my @wordlist1=split ":",$line;
	my @wordlist2=split "=",$wordlist1[2];

	return $wordlist2[1];
}
sub get_sys_call_keywords
{
	my $line=shift;
	my $syscallno=shift;
	my $ret_str;
	
	my $syscall_enter_str=SYSCALL_ENTER.":";
	my $syscall_exit_str=SYSCALL_EXIT.":";
	
	if($syscallno==(SYS_RT_SIGRETURN_WRAPPER))
	{
		$ret_str=KEY_SYS_RT_SIGRETURN_WRAPPER;
	}
	if($syscallno==(SYS_RT_SIGTIMEDWAIT))
	{
		$ret_str=KEY_SYS_RT_SIGTIMEDWAIT;
	}	
	if($syscallno==(SYS_RT_SIGSUSPEND))
	{
		$ret_str=KEY_SYS_RT_SIGSUSPEND;
	}
	if($line=~m/$syscall_enter_str/)
	{
		$ret_str=KEY_SYS_ENTER.$ret_str;		
	}	
	if($line=~m/$syscall_exit_str/)
	{
		$ret_str=KEY_SYS_EXIT.$ret_str;			
	}
	return $ret_str;
}
sub show_usage
{
	print "Usage:\n";
	print "parse_kcu.pl trace_file";
}
sub preproc_input_file
{
	my $filename=shift;
	my $input_fn;
	my $timestamp;
	my $task;
	my $sigindex=0;
	my @sig_gen_info;
	my $syscallno;
	my $sig_gen_str=SIG_GENERATE.":";
	my $sig_del_str=SIG_DELIVER.":";
	my $syscall_enter_str=SYSCALL_ENTER.":";
	my $syscall_exit_str=SYSCALL_EXIT.":";
	my $firstsiggen=0;		
			
	open($input_fn,"<$filename") || die "can not open $filename for read\n";
	open(my $out_task_time_fn,">".(ALL_TASK_VS_TIME)) || die "can not open ". (ALL_TASK_VS_TIME)." for write\n";	
	
	
	while(my $line=<$input_fn>)
	{
		$timestamp=get_timestamp($line)*(TIME_STAMP_MUXER);
		if(($firstsiggen==0) and !($line=~m/$sig_gen_str/))
		{
			#printf "skip $line...\n";
			next;
		}
		$firstsiggen=1;	
		if($line=~m/$sig_gen_str/)
		{
			#printf "match signal signal generate\n";
			$task=get_task_name($line);
			++$sigindex;
			save_signal_generate_info($task,$sigindex,$timestamp);
			printf $out_task_time_fn "%d:%s:%s%d\n",$sigindex,$task,$sig_gen_str,$timestamp;
		}
		if($line=~m/$sig_del_str/)
		{
			$task=get_curr_task_name($line);			
			@sig_gen_info=get_signal_generate_info($task,1);
			printf $out_task_time_fn "%d:%s:%s%d\n",$sig_gen_info[1],$task,$sig_del_str,$timestamp;
			save_sig_info_by_task($task,$sig_gen_info[0],$sig_gen_info[1],$sig_gen_str);
		}
		if(($line=~m/$syscall_enter_str/) or ($line=~m/$syscall_exit_str/))		
		{
			$task=get_curr_task_name($line);
			$syscallno=get_syscall_no($line);
			
			if(($syscallno==(SYS_RT_SIGTIMEDWAIT)) and ($line=~m/$syscall_exit_str/))#exit sigwaitinfo but no signal caught
			{
				if(get_syscall_ret($line)!=(EXP_SIG_NO))#filter the ret from sigwaitinfo
				{
					#printf "unexpected syscall ret:%d\n",get_syscall_ret($line);
					next;
				}
			}
			if(($syscallno==(SYS_RT_SIGRETURN_WRAPPER)) and ($line=~m/$syscall_exit_str/))#exit from rt_sig_ret is not cared
			{
				next;
			}
			if(($syscallno==(SYS_RT_SIGSUSPEND)) and ($line=~m/$syscall_enter_str/))#enter rt_sigsuspend is not cared
			{
				next;
			}
			
			if(($syscallno==(SYS_RT_SIGSUSPEND)) and ($line=~m/$syscall_exit_str/))#ret from sigsuspend is after sig generate
			{
				@sig_gen_info=get_signal_generate_info($task,0);
			}
			else
			{
				@sig_gen_info=get_sig_info_by_task($task,$sig_gen_str);			
			}

			if($sig_gen_info[1])
			{
				printf $out_task_time_fn "%d:%s:%s:%d\n",$sig_gen_info[1],$task,get_sys_call_keywords($line,$syscallno),$timestamp;
			}
		}
	}
	close($out_task_time_fn);	
	close($input_fn);
	
	return (ALL_TASK_VS_TIME);
}
sub split_file_by_sync_signal
{
	my $filename=shift;
	my $split_sync=shift;
	my @arg_list;
	my $out_filename;
	
	open(my $input_fn,"<$filename") || die "can not open $filename for read\n";	
	if($split_sync)
	{
		$out_filename=SYNC_SIGNAL_VS_TIME;
	}
	else
	{
		$out_filename=ASYNC_SIGNAL_VS_TIME;		
	}
	open(my $output_fn,">$out_filename") || die "can not open $out_filename for write\n";
	
	while(my $line=<$input_fn>)
	{
		@arg_list=split ":",$line;
		if($split_sync and is_sync_task($arg_list[1]))
		{
			printf $output_fn $line;
		}
		else
		{
			if(!$split_sync and !is_sync_task($arg_list[1]))
			{
				printf $output_fn $line;
			}				
		}
	}
	
	close($output_fn);
	close($input_fn);
	
	return $out_filename;
}
sub sort_by_1st_field_num
{
	my @list1=split ":",$a;
	my @list2=split ":",$b;	
	return $list1[0]<=>$list2[0];
}

my $figure_idx=1;
sub gen_m_sig_vs_time
{
	my $filename=shift;
	my $sync_thd=shift;
	my $sig_gen_str=SIG_GENERATE;
	my $sig_del_str=SIG_DELIVER;
	my $syscall_enter_sigret_str=KEY_SYS_ENTER.KEY_SYS_RT_SIGRETURN_WRAPPER;
	my $syscall_exit_sigret_str=KEY_SYS_EXIT.KEY_SYS_RT_SIGRETURN_WRAPPER;
	my $syscall_enter_sigwait_str=KEY_SYS_ENTER.KEY_SYS_RT_SIGTIMEDWAIT;
	my $syscall_exit_sigwait_str=KEY_SYS_EXIT.KEY_SYS_RT_SIGTIMEDWAIT;
	my $syscall_enter_sigsusp_str=KEY_SYS_ENTER.KEY_SYS_RT_SIGSUSPEND;
	my $syscall_exit_sigsusp_str=KEY_SYS_EXIT.KEY_SYS_RT_SIGSUSPEND;
	my @arg_list;
	my $lasty=0;
	my $starttime;			
	my $sigcnt=0;
	my $sigstart=0;
	my $sigend=0;	
	my $deltatime=0;
	my $maxdeltatime=0;
	my $mindeltatime=0;	
	my $timerange=0;
	my $laststate=0;
	$filename=~m/(\w+)\.\w+/;
	
	my $outfilename=$1.".m";
	
	#print "out:$outfilename\n";
	
	open(my $input_fn,"<$filename") || die "can not open $filename for read\n";
	open(my $output_fn,">$outfilename") || die "can not open $outfilename for write\n";
	
    printf $output_fn "format long;\n";
	printf $output_fn "figure(%d);\ncla;\n",$figure_idx++;
	
	
	#sync signal sequence:signal_generate->ret_from_sys_rt_sigsuspend->signal_deliver->enter_in_sys_rt_sig_ret->ret_from_sys_rt_sig_ret->enter_in_sys_rt_sigsuspend
	#async signal sequence:signal_generate->signal_deliver->ret_from_sys_rt_sigwait->enter_in_sys_rt_sigwait
	#printf $output_fn "hold on;\n";
	#printf $output_fn "y=0;\n";
	#printf $output_fn "x=0;\n";
	#printf $output_fn "plot(x,y,'linewidth',3,'color',[0,1,0]);\n";	
	
	while(my $line=<$input_fn>)
	{
		@arg_list=split ":",$line;#sigindex task stage time
		
		if($arg_list[2] eq $sig_gen_str)
		{
			$starttime=$arg_list[3];
			$lasty=0;
			$sigcnt++;
			if($sigstart==0)
			{
				$sigstart=$arg_list[0];
			}
			if($timerange==0)
			{
				$timerange=$starttime;
			}
			$sigend=$arg_list[0];
			$laststate=0;
			next;
		}
		if($arg_list[2] eq $sig_del_str)
		{
			$deltatime=$arg_list[3]-$starttime;
			printf $output_fn "hold on;\n";
			printf $output_fn "y=%d:1:%d;\n",$lasty,$deltatime;
			printf $output_fn "x=(y<=%d).*%d;\n",$deltatime,$arg_list[0];
			printf $output_fn "plot(x,y,'linewidth',3,'color',[0,0,1]);\n";
			$lasty=$deltatime;
			if($sync_thd and ($laststate!=2))
			{
				printf "no exit suspend:%s %d us\n",$arg_list[1],$arg_list[3];
			}
			$laststate=1;
		}
		if($arg_list[2] eq $syscall_exit_sigsusp_str)#sync signal will wake up task and exit signal suspend after receive signal
		{
			$deltatime=$arg_list[3]-$starttime;
			printf $output_fn "hold on;\n";
			printf $output_fn "y=%d:1:%d;\n",$lasty,$deltatime;
			printf $output_fn "x=(y<=%d).*%d;\n",$deltatime,$arg_list[0];
			printf $output_fn "plot(x,y,'linewidth',3,'color',[0,1,0]);\n";
			$lasty=$deltatime;
			$laststate=2;
		}
		if($arg_list[2] eq $syscall_enter_sigret_str)#sync signal handler finished
		{
			$deltatime=$arg_list[3]-$starttime;
			printf $output_fn "hold on;\n";
			printf $output_fn "y=%d:1:%d;\n",$lasty,$deltatime;
			printf $output_fn "x=(y<=%d).*%d;\n",$deltatime,$arg_list[0];
			printf $output_fn "plot(x,y,'linewidth',3,'color',[0,1,1]);\n";
			$lasty=$deltatime;			
		}
		if($arg_list[2] eq $syscall_exit_sigret_str)#sync signal handler ret finished
		{
			$deltatime=$arg_list[3]-$starttime;
			printf $output_fn "hold on;\n";
			printf $output_fn "y=%d:1:%d;\n",$lasty,$deltatime;
			printf $output_fn "x=(y<=%d).*%d;\n",$deltatime,$arg_list[0];
			printf $output_fn "plot(x,y,'linewidth',3,'color',[1,0,0]);\n";
			$lasty=$deltatime;			
		}
		if($arg_list[2] eq $syscall_enter_sigsusp_str)#sync signal enter sigsuspend again
		{
			$deltatime=$arg_list[3]-$starttime;
			printf $output_fn "hold on;\n";
			printf $output_fn "y=%d:1:%d;\n",$lasty,$deltatime;
			printf $output_fn "x=(y<=%d).*%d;\n",$deltatime,$arg_list[0];
			printf $output_fn "plot(x,y,'linewidth',3,'color',[1,0,1]);\n";
			$lasty=$deltatime;
		}
		if($arg_list[2] eq $syscall_exit_sigwait_str)#async ret from sigwaitinfo after signal deliver
		{
			$deltatime=$arg_list[3]-$starttime;
			printf $output_fn "hold on;\n";
			printf $output_fn "y=%d:1:%d;\n",$lasty,$deltatime;
			printf $output_fn "x=(y<=%d).*%d;\n",$deltatime,$arg_list[0];
			printf $output_fn "plot(x,y,'linewidth',3,'color',[1,1,0]);\n";
			$lasty=$deltatime;				
		}	
		if($arg_list[2] eq $syscall_enter_sigwait_str)#async handle signal finished,enter sigwaitinfo again
		{
			$deltatime=$arg_list[3]-$starttime;
			printf $output_fn "hold on;\n";
			printf $output_fn "y=%d:1:%d;\n",$lasty,$deltatime;
			printf $output_fn "x=(y<=%d).*%d;\n",$deltatime,$arg_list[0];
			printf $output_fn "plot(x,y,'linewidth',3,'color',[0.5,0.5,0]);\n";
			$lasty=$deltatime;
		}
			
		if($maxdeltatime==0)
		{
			$maxdeltatime=$deltatime;
		}
		if($mindeltatime==0)
		{
			$mindeltatime=$deltatime;
		}
		if($deltatime>$maxdeltatime)
		{
			$maxdeltatime=$deltatime;
		}
		if($deltatime<$mindeltatime)
		{
			$mindeltatime=$deltatime;
		}
	}
	if($timerange==0)#empty
	{
		close($output_fn);
		close($input_fn);
		unlink $outfilename;
		return "";
	}
	
	$timerange=$arg_list[3]-$timerange;
	my $xstep=($sigend-$sigstart+20)/40;#40points
	my $ystep=($maxdeltatime+20)/40;#40points	
	
	if($xstep<1)
	{
		$xstep=1;
	}
	if($ystep<1)
	{
		$ystep=1;
	}
	if($sigend==0)
	{
		$sigend=1;
	}
	if($maxdeltatime==0)
	{
		$maxdeltatime=1;
	}

	
	printf $output_fn "axis([%d %d 0 %d]);\n",$sigstart,$sigend,$maxdeltatime;
	printf $output_fn "set(gca,'xTick',[%d:%d:%d]);\n",$sigstart,$xstep,$sigend;
	printf $output_fn "set(gca,'yTick',[0:%d:%d]);\n",$ystep,$maxdeltatime;	
	printf $output_fn "grid on;\n";
	printf $output_fn "ylabel('time(us) %d/div');\n",$ystep;
	printf $output_fn "xlabel('signal %d/div');\n",$xstep;
	#signal_generate->ret_from_sys_rt_sigsuspend->signal_deliver->enter_in_sys_rt_sig_ret->ret_from_sys_rt_sig_ret->enter_in_sys_rt_sigsuspend
	#async signal sequence:signal_generate->signal_deliver->ret_from_sys_rt_sigwait->enter_in_sys_rt_sigwait	
	if($sync_thd)
	{
		printf $output_fn "title('sync signal vs time min:%d us max:%d us in %d signals time range:%d us');\n",$mindeltatime,$maxdeltatime,$sigcnt,$timerange;	
		printf $output_fn "legend('ret sysrtsigsuspend','signal deliver','sighand ret');\n";
	}
	else
	{
		printf $output_fn "title('async signal vs time min:%d us max:%d us in %d signals time range:%d us');\n",$mindeltatime,$maxdeltatime,$sigcnt,$timerange;	
		printf $output_fn "legend('signal deliver','ret sysrtsigwait','enter sysrtsigwait');\n";		
	}
	close($output_fn);
	close($input_fn);
	
	return $outfilename;			
}
sub sort_input_file
{
	my $filename=shift;
	$filename=~m/(\w+)\.\w+/;
	my $outfilename=$1."_sort";
	my @file_cont;	
		
	open(my $input_fn,"<$filename") || die "can not open $filename for read\n";
	@file_cont=<$input_fn>;
	close($input_fn);
	@file_cont=sort sort_by_1st_field_num @file_cont;
	open(my $output_fn,">$outfilename") || die "can not open $outfilename for write\n";
	foreach my $index (0 .. $#file_cont)
	{
		printf $output_fn $file_cont[$index];
	}
	close($output_fn);
	return $outfilename;
}
sub gen_m_task_vs_time
{
	my $filename=shift;
	my $thdname=shift;
	my $outfilename=$thdname."_vs_time.m";
	my @arg_list;
	my $lasttime=0;
	my $deltatime=0;
	my $basetime=0;
	my $newtime=0;
	my $offsettime=0;
	my $sig_gen_str=SIG_GENERATE;
	my $sig_del_str=SIG_DELIVER;
	my $syscall_enter_sigret_str=KEY_SYS_ENTER.KEY_SYS_RT_SIGRETURN_WRAPPER;
	my $syscall_exit_sigret_str=KEY_SYS_EXIT.KEY_SYS_RT_SIGRETURN_WRAPPER;
	my $syscall_enter_sigwait_str=KEY_SYS_ENTER.KEY_SYS_RT_SIGTIMEDWAIT;
	my $syscall_exit_sigwait_str=KEY_SYS_EXIT.KEY_SYS_RT_SIGTIMEDWAIT;
	my $syscall_enter_sigsusp_str=KEY_SYS_ENTER.KEY_SYS_RT_SIGSUSPEND;
	my $syscall_exit_sigsusp_str=KEY_SYS_EXIT.KEY_SYS_RT_SIGSUSPEND;
	my $lasty=0;
	my $newy=0;
	my $linecorlor;
	my $mintime=0;
	my $maxtime=0;
	my $starttime=0;
	my $mindeltatime=0;
	my $maxdeltatime=0;	
	my $maxy=0;	
	my $xstep=0;	
	my $sigcnt=0;	
	my $ng_sig_cnt=0;	
	my $lastsigdonetime=0;
	my $lastsigng=0;
	my $timerange=0;
	my $syncthd=is_sync_task($thdname);
		
	$outfilename=~s/\///;	
	open(my $input_fn,"<$filename") || die "can not open $filename for read\n";
	open(my $output_fn,">$outfilename") || die "can not open $outfilename for write\n";
	
	printf $output_fn "format long;\n";
	printf $output_fn "figure(%d);\ncla;\n",$figure_idx++;
	
	while(my $line=<$input_fn>)
	{
		@arg_list=split ":",$line;#sigindex task stage time
		
		if($arg_list[1] ne $thdname)
		{
			next;
		}
		if($arg_list[2] eq $sig_gen_str)
		{
			if($timerange==0)
			{
				$timerange=$arg_list[3];
			}	
			if($basetime==0)
			{
				$basetime=$arg_list[3];
				#printf "basetime:$basetime\n";
			}
			if($lastsigdonetime and ($arg_list[3]<$lastsigdonetime))
			{
				$ng_sig_cnt++;
				$lastsigng=1;
				#printf "$thdname:ng_signal:now $arg_list[3]:last:$lastsigdonetime\n";
			}
			else
			{
				$lastsigng=0;
			}
			if($lasttime and ($arg_list[3]-$lasttime)>10)
			{
				$lasttime=$lasttime+10;
			}
			else
			{
				$lasttime=$arg_list[3];
			}
			$deltatime=$arg_list[3]-$lasttime;
			#printf ""
			$lasty=0;
			$starttime=$arg_list[3];
			$sigcnt++;
			next;
		}
		if($arg_list[2] eq $sig_del_str)
		{
			$newtime=$arg_list[3]-$deltatime;
			$offsettime=$lasttime-$basetime;
			if($lastsigng)
			{
				$linecorlor="[0,0,0]";
			}
			else
			{
				$linecorlor="[0,0,1]";
			}	
			$lasttime=$newtime;
			$lastsigdonetime=$arg_list[3];#treat async signal dequeue as finished time
			if($syncthd)
			{
				$newy=2;
			}
			else
			{
				$newy=1;
			}
		}
		if($arg_list[2] eq $syscall_exit_sigsusp_str)
		{
			$newtime=$arg_list[3]-$deltatime;
			$offsettime=$lasttime-$basetime;
			$linecorlor="[0,1,0]";
			$lasttime=$newtime;
			if($syncthd)
			{
				$newy=1;
			}
			else#un-expected case
			{
				$newy=0;
			}			
		}
		if($arg_list[2] eq $syscall_enter_sigret_str)#sync signal handler finished
		{
			$newtime=$arg_list[3]-$deltatime;
			$offsettime=$lasttime-$basetime;
			$linecorlor="[0,1,1]";
			$lasttime=$newtime;	
			$lastsigdonetime=$arg_list[3];#sync signal handler finished
			if($syncthd)
			{
				$newy=3;
			}
			else#un-expected case
			{
				$newy=0;
			}
		}
		if($arg_list[2] eq $syscall_exit_sigret_str)#sync signal handler ret finished
		{
			$newtime=$arg_list[3]-$deltatime;
			$offsettime=$lasttime-$basetime;
			$linecorlor="[1,0,0]";
			$lasttime=$newtime;	
			if($syncthd)
			{
				$newy=4;
			}
			else#un-expected case
			{
				$newy=0;
			}		
		}
		if($arg_list[2] eq $syscall_enter_sigsusp_str)#sync signal enter sigsuspend again
		{
			$newtime=$arg_list[3]-$deltatime;
			$offsettime=$lasttime-$basetime;
			$linecorlor="[1,0,1]";
			$lasttime=$newtime;
			if($syncthd)
			{
				$newy=5;
			}
			else#un-expected case
			{
				$newy=0;
			}			
		}
		if($arg_list[2] eq $syscall_exit_sigwait_str)#async ret from sigwaitinfo after signal deliver
		{
			$newtime=$arg_list[3]-$deltatime;
			$offsettime=$lasttime-$basetime;
			$linecorlor="[1,0,0]";
			$lasttime=$newtime;	
			if($syncthd)#un-expected case
			{
				$newy=0;
			}
			else
			{
				$newy=2;
			}			
		}
		if($arg_list[2] eq $syscall_enter_sigwait_str)#async handle signal finished,enter sigwaitinfo again
		{
			$newtime=$arg_list[3]-$deltatime;
			$offsettime=$lasttime-$basetime;
			$linecorlor="[1,0,1]";
			$lasttime=$newtime;
			if($syncthd)#un-expected case
			{
				$newy=0;
			}
			else
			{
				$newy=2;
			}							
		}
		
		if(($maxtime==0) or ($maxtime<($newtime-$basetime)))
		{
			$maxtime=$newtime-$basetime;
		}
		if(($mintime>($newtime-$basetime)))
		{
			$mintime=$newtime-$basetime;
		}
		
		if(($maxdeltatime==0) or ($maxdeltatime<($arg_list[3]-$starttime)))
		{
			$maxdeltatime=$arg_list[3]-$starttime;
		}
		if(($mindeltatime==0) or ($mindeltatime>($arg_list[3]-$starttime)))
		{
			$mindeltatime=$arg_list[3]-$starttime;
		}
				
		printf $output_fn "hold on;\n";
		printf $output_fn "x=[%d %d];\n",$offsettime,$newtime-$basetime;
		printf $output_fn "y=[%d %d];\n",$lasty,$newy;
		printf $output_fn "plot(x,y,'linewidth',3,'color',%s);\n",$linecorlor;	
		$lasty=$newy;
		
		if($lasty>$maxy)
		{
			$maxy=$lasty;
		}
	}
	if($timerange==0)#empty
	{
		close($output_fn);
		close($input_fn);
		unlink $outfilename;
		return "";
	}
	$timerange=$arg_list[3]-$timerange;
	$xstep=($maxtime-$mintime)/100;#100points
	if($xstep<1)
	{
		$xstep=1;
	}
	if($maxtime==0)
	{
		$maxtime=1;
	}
	if($maxy==0)
	{
		$maxy=1;
	}	

	
	printf $output_fn "axis([%d %d 0 %d]);\n",$mintime,$maxtime,$maxy;
	printf $output_fn "set(gca,'xTick',[%d:%d:%d]);\n",$mintime,$xstep,$maxtime;
	printf $output_fn "set(gca,'yTick',[0:1:%d]);\n",$maxy;	
	printf $output_fn "grid on;\n";
	printf $output_fn "ylabel('stage');\n";
	printf $output_fn "xlabel('time %d us/div');\n",$xstep;
	printf $output_fn "title('%s vs time busy %d/%d min:%d us max:%d us range:%d us');\n",$thdname,$ng_sig_cnt,$sigcnt,$mindeltatime,$maxdeltatime,$timerange;	
	
	if($syncthd)
	{
		printf $output_fn "set(gca,'yTicklabel','signal generate|ret sysrtsigsuspend|signal deliver|sig hand ret')\n";
		printf $output_fn "legend('ret sysrtsigsuspend','signal deliver','sighand ret');\n";
	}
	else
	{
		printf $output_fn "set(gca,'yTicklabel','signal generate|signal deliver|ret sysrtsigwait|enter sysrtsigwait')\n";	
		printf $output_fn "legend('signal deliver','ret sysrtsigwait','enter sysrtsigwait');\n";		
	}
	
	close($output_fn);
	close($input_fn);
	return $outfilename;
}
if($#ARGV<0)
{
	show_usage();
	exit;
}
my $output_filename=preproc_input_file(@ARGV);
my $output_sort_filename=sort_input_file($output_filename);

my $output_s_sig_vs_time=split_file_by_sync_signal($output_sort_filename,1);#sync signal
my $output_a_sig_vs_time=split_file_by_sync_signal($output_sort_filename,0);#async signal
my $output_s_sig_vs_time_m=gen_m_sig_vs_time($output_s_sig_vs_time,1);#sync signal
my $output_a_sig_vs_time_m=gen_m_sig_vs_time($output_a_sig_vs_time,0);#async signal

my $output_s_task0_vs_time_m=gen_m_task_vs_time($output_s_sig_vs_time,(SYNC_THD0_NAME));#sync0 signal
my $output_s_task1_vs_time_m=gen_m_task_vs_time($output_s_sig_vs_time,(SYNC_THD1_NAME));#sync1 signal
my $output_s_task2_vs_time_m=gen_m_task_vs_time($output_s_sig_vs_time,(SYNC_THD2_NAME));#sync2 signal
my $output_s_task3_vs_time_m=gen_m_task_vs_time($output_s_sig_vs_time,(SYNC_THD3_NAME));#sync3 signal
my $output_a_task0_vs_time_m=gen_m_task_vs_time($output_a_sig_vs_time,(ASYNC_THD0_NAME));#sync0 signal
my $output_a_task1_vs_time_m=gen_m_task_vs_time($output_a_sig_vs_time,(ASYNC_THD1_NAME));#sync1 signal

printf "output:\n$output_s_task0_vs_time_m\n$output_s_task1_vs_time_m\n$output_s_task2_vs_time_m\n$output_s_task3_vs_time_m\n";
printf "$output_a_task0_vs_time_m\n$output_a_task1_vs_time_m\n";
