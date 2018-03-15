#!/usr/bin/perl
use strict;
use warnings;

sub construct_api
{
    (my $h_file,my $out_file,my $out_usr_file)=@_;
    my $func_ret;
    my $func_name;
    my $func_args;
    my @input_arg_list;
    my @output_arg_list;
    my @tmp_list;
    my @struct_def;
    my $struct_name;
    my $ret_func_cnt=0;
    my $ret_func_name;
    my $nfy_id;
    my $input_arg_name;
    my $input_arg_len;
    my $output_arg_name;
    my $output_arg_len;    
    my @nfy_id_list;
    my @register_list;
    my $tmp_val;
    my @usr_input_arg_list;
    my @usr_output_arg_list;
    my @usr_output_rem_code;
    my @usr_output_rem_code2;
    open(my $input_h_fn,"<$h_file") || die "can not open $h_file for read\n";
    open(my $out_fn,">$out_file") || die "can not open $out_file for write\n";
    open(my $out_usr_fn,">$out_usr_file") || die "can not open $out_usr_file for write\n";
    
    printf $out_fn "#include <linux/soc/mediatek/kcu.h>\n";
    printf $out_usr_fn "#include <stddef.h>\n";    
    printf $out_usr_fn "#include \"kcu.h\"\n";
    printf "please include the $h_file into $out_usr_file and $out_file\n";
    
    while(my $line=<$input_h_fn>)
    {
        if($line=~m/^[^#]/)
        {
            if($line=~m/^\s*(\w+\**)\s+(\w+)\((.*)\);/)
            {
                #print "1:$1 2:$2 3:$3 line:$line\n";
                $func_ret=$1;
                $func_name=$2;
                $func_args=$3;
                $ret_func_cnt=0;
                $ret_func_name="NON_RET";
                
                #printf("func_args:$func_args\n");
                
                splice @input_arg_list,0;#clear
                splice @output_arg_list,0;    #clear  
                splice @usr_input_arg_list,0;#clear
                splice @usr_output_arg_list,0;    #clear  
                splice @usr_output_rem_code,0;    #clear 
                splice @usr_output_rem_code2,0;    #clear                  
                
                if($func_args eq "void" or $func_args eq "VOID" or $func_args=~m/^\s*$/ )
                {
                  
                }
                else
                {
                    @tmp_list=split ",",$func_args;
                    my @type_value_list_tmp;
                    foreach my $index(0 .. $#tmp_list)
                    {
                        if($tmp_list[$index]=~m/IN_ONLY/)
                        {
                            $tmp_list[$index]=~s/IN_ONLY//;
                            $tmp_list[$index]=~s/\*//;
                            @type_value_list_tmp=split " ",$tmp_list[$index];
                            push @input_arg_list,"$type_value_list_tmp[0]*  $type_value_list_tmp[1]";
                            #printf("11:$type_value_list_tmp[0] $type_value_list_tmp[1]\n");
                        }
                        else
                        {
                            if($tmp_list[$index]=~m/OUT_ONLY/)    
                            {
                                $tmp_list[$index]=~s/OUT_ONLY//;
                                $tmp_list[$index]=~s/\*//;
                                @type_value_list_tmp=split " ",$tmp_list[$index];
                                push @output_arg_list,"$type_value_list_tmp[0]*  $type_value_list_tmp[1]";
                                #printf("22:$type_value_list_tmp[0] $type_value_list_tmp[1]\n");                                
                            }
                            else
                            {
                                if($tmp_list[$index]=~m/\*/)#both in and out pointer
                                {
                                    $tmp_list[$index]=~s/\*//;
                                    @type_value_list_tmp=split " ",$tmp_list[$index];
                                    push @output_arg_list,"$type_value_list_tmp[0]*  $type_value_list_tmp[1]";
                                    push @input_arg_list,"$type_value_list_tmp[0]*  $type_value_list_tmp[1]";                                     
                                   # printf("33:$type_value_list_tmp[0] $type_value_list_tmp[1]\n");
                                }
                                else#just value,input
                                {
                                    @type_value_list_tmp=split " ",$tmp_list[$index];
                                    push @input_arg_list,"$type_value_list_tmp[0] $type_value_list_tmp[1]";
                                    #printf("44:$type_value_list_tmp[0] $type_value_list_tmp[1]\n");                                    
                                }
                            }
                        }
                    }
                }
                if($func_ret ne "void")
                {
                    $ret_func_cnt=1;
                    push @output_arg_list,"$func_ret func_ret";
                }
                $nfy_id="NFY_".uc($func_name);
                push @nfy_id_list,$nfy_id;
                
                printf $out_fn "%s %s(%s)\n{\n",$func_ret,$func_name,$func_args;                
                printf $out_fn "\tint ret;\n";  
                
                        #output usr file
            push @register_list,"kcu_register_nfyid($nfy_id,1,kcu_$func_name,NULL)";
            printf $out_usr_fn "static void kcu_%s(void * pvArg)\n{\n",$func_name;
            #printf $out_usr_fn "void kcu_%s(void * pvArg)\n{\n",$func_name;
        
                ##########input arg process ###################
                if($#input_arg_list>0)
                {
                    $struct_name=uc($func_name)."_INFO";
                    push @struct_def,"typedef struct\n{\n";
                    
                    printf $out_usr_fn "\t$struct_name *ptr_input=($struct_name *)pvArg;\n";
                    
                    printf $out_fn "\t$struct_name input_arg=\n\t{";
                    my $iter_idx=0;
                    foreach(@input_arg_list)
                    {
                        @tmp_list=split " ",$_;
                       # printf("55:$tmp_list[0]==$tmp_list[1]\n");
                        if($iter_idx!=0)
                        {
                            printf $out_fn ",";
                        }
                        $iter_idx++;
                        if($tmp_list[0]=~m/\*/)
                        {
                             push @usr_input_arg_list,"&(ptr_input->$tmp_list[1])";
                             printf $out_fn "\n\t*$tmp_list[1]";
                        }
                        else
                        {
                             push @usr_input_arg_list,"ptr_input->$tmp_list[1]";
                             printf $out_fn "\n\t$tmp_list[1]";                            
                        }
                        $tmp_list[0]=~s/\*//;
                        push @struct_def,"\t$tmp_list[0] $tmp_list[1];\n"; 
                    }
                    push @struct_def,"}$struct_name;\n\n";
                    printf $out_fn "\n\t};\n";
                    $input_arg_name="&input_arg";
                    $input_arg_len="sizeof(input_arg)";
                }
                else
                {
                    if($#input_arg_list==0)
                    {
                        @tmp_list=split " ",$input_arg_list[0];
                        if($tmp_list[0]=~m/\*/)
                        {
                            # printf $out_usr_fn "\t$struct_name *ptr_input=($struct_name *)pvArg;\n";
                            push @usr_input_arg_list,"($tmp_list[0])pvArg";
                            $input_arg_name="$tmp_list[1]";
                            $input_arg_len="sizeof(*$tmp_list[1])";                              
                        }
                        else
                        {
                            push @usr_input_arg_list,"*($tmp_list[0] *)pvArg";
                            $input_arg_name="&$tmp_list[1]";
                            $input_arg_len="sizeof($tmp_list[1])";                             
                        }
                    }
                    else
                    {
                            $input_arg_name="NULL";
                            $input_arg_len="0";                         
                    }
                }
                ######################input arg process End################
                ##########output arg process ###################
                if($#output_arg_list>0)
                {
                     $struct_name="RET_".uc($func_name)."_INFO";
                    push @struct_def,"typedef struct\n{\n";
                    
                    printf $out_usr_fn "\t$struct_name output_arg;\n";
                    
                    printf $out_fn "\t$struct_name output_arg;\n";
                    foreach(@output_arg_list)
                    {
                        @tmp_list=split " ",$_;
                        $tmp_list[0]=~s/\*//;
                        push @usr_output_arg_list,"&(output_arg.$tmp_list[1])";
                        push @struct_def,"\t$tmp_list[0] $tmp_list[1];\n";
                        if($tmp_list[1] eq "func_ret")
                        {
                            if($func_ret=~m/\*/)
                            {
                                push @usr_output_rem_code,"\tmemcpy(&(output_arg.func_ret),ret,sizeof(*ret));\n";
                            }
                            else
                            {
                                push @usr_output_rem_code,"\tmemcpy(&(output_arg.func_ret),&ret,sizeof(ret));\n"                                
                            }
                        }
                    }
                    push @struct_def,"}$struct_name;\n\n";
                    $output_arg_name="&output_arg";
                    $output_arg_len="sizeof(output_arg)";
                    push @usr_output_rem_code,"\tmemcpy(pvArg,&output_arg,sizeof(output_arg));\n";
                }
                else
                {
                    if($#output_arg_list==0)#only one output
                    {
                        if($ret_func_cnt>0)
                        {
                            @tmp_list=split " ",$output_arg_list[0];
                            $tmp_list[0]=~s/\*//;
                            printf $out_fn "\t$tmp_list[0] $tmp_list[1];\n";
                            $output_arg_name="&$tmp_list[1]";
                            $output_arg_len="sizeof($tmp_list[1])";
                            if($func_ret=~m/\*/)
                            {
                                push @usr_output_rem_code,"\tmemcpy(pvArg,ret,sizeof(*ret));\n";
                            }
                            else
                            {
                                push @usr_output_rem_code,"\tmemcpy(pvArg,&ret,sizeof(ret));\n"                                
                            }
                        }
                        else
                        {
                            @tmp_list=split " ",$output_arg_list[0];
                            $tmp_list[0]=~s/\*//;
                            #printf $out_fn "$tmp_list[0] $tmp_list[1];\n";
                            $output_arg_name="$tmp_list[1]";
                            $output_arg_len="sizeof(*$tmp_list[1])";
                            printf $out_usr_fn "\t$tmp_list[0] $tmp_list[1];\n";
                            push @usr_output_arg_list,"&$tmp_list[1]";
                            push @usr_output_rem_code,"\tmemcpy(pvArg,&$tmp_list[1],sizeof(tmp_list[1]));\n";
                        }
                    }
                    else#non output
                    {
                            $output_arg_name="NULL";
                            $output_arg_len="0";                         
                    }
                }
                
                if($func_ret=~m/\*/)
                {
                    $tmp_val=$func_ret;
                    $tmp_val=~s/\*//;
                    printf $out_fn "\t$func_ret ptr_func_ret=($func_ret)kmalloc(sizeof($tmp_val),GFP_KERNEL);\n";
                }
                ######################output arg process End################
                
                printf $out_fn "\tret=kcu_notify_sync(%s,%s,%s,%s,%s);\n",$nfy_id,$input_arg_name,$input_arg_len,$output_arg_name,$output_arg_len;
                printf $out_fn "\tif(ret!=0)\n\t{\n\t\tprintk(\"call nfy_id %%d fails,ret=%%d\\n\",%s,ret);\n\t}\n",$nfy_id;
                
                if($#output_arg_list>0)
                {
                    foreach(@output_arg_list)
                    {
                        @tmp_list=split " ",$_;
                        #$tmp_list[0]=~s/\*//;
                        if($tmp_list[1] eq "func_ret")
                        {
                             if($func_ret=~m/\*/)#return allocated buffer
                             {
                                printf $out_fn "\tmemcpy(ptr_func_ret,&output_arg.$tmp_list[1],sizeof(*ptr_func_ret));\n";
                                $ret_func_name="ptr_func_ret";
                             }
                             else
                             {
                                $ret_func_name="output_arg.$tmp_list[1]";
                             } 
                            next;
                        }
                        printf $out_fn "\t*$tmp_list[1]=output_arg.$tmp_list[1];\n";
                        #$ret_func_name="output_arg.func_ret";
                    }
                }
                else
                {
                    if($#output_arg_list==0)#only one output
                    {
                        if($ret_func_cnt>0)
                        {
                            @tmp_list=split " ",$output_arg_list[0];
                             if($func_ret=~m/\*/)#return allocated buffer
                             {
                                printf $out_fn "\tmemcpy(ptr_func_ret,&$tmp_list[1],sizeof($tmp_list[1]));\n";
                                $ret_func_name="ptr_func_ret";
                             }
                             else
                             {
                                $ret_func_name="$tmp_list[1]";
                             }
                        }
                    }
                }
                if($ret_func_name ne "NON_RET")
                {
                    printf $out_fn "\treturn $ret_func_name;\n"; 
                }
                printf $out_fn "}\n";
        
        #output usr file
       # push @register_list,"kcu_register_nfyid($nfy_id,1,kcu_$func_name,NULL)";
        #printf $out_usr_fn "void kcu_%s(void * pvArg)\n{\n",$func_name;
        if($func_ret ne "void")
        {
            printf $out_usr_fn "\t$func_ret ret;\n";
            printf $out_usr_fn "\tret=%s(",$func_name;
        }
        else
        {
            printf $out_usr_fn "\t%s(",$func_name;
        }
        
        if($func_args eq "void" or $func_args eq "VOID" or $func_args=~m/^\s*$/)
        {
                
        }
        else
        {
        @tmp_list=split ",",$func_args;
        
        foreach my $index(0 .. $#tmp_list)
        {
            my @tmp_remove;
            my @tmp_remove2;
            if($index!=0)
            {
                printf $out_usr_fn ",";
            }
            if($tmp_list[$index]=~m/OUT_ONLY/)
            {
                @tmp_remove=splice @usr_output_arg_list,0,1;
                printf $out_usr_fn "%s",$tmp_remove[0];
                next;
            }
            if($tmp_list[$index]=~m/IN_ONLY/)
            {
                @tmp_remove=splice @usr_input_arg_list,0,1;
                printf $out_usr_fn "%s",$tmp_remove[0];
                next;
            }
            if($tmp_list[$index]=~m/\*/)#in_out
            {
                @tmp_remove=splice @usr_input_arg_list,0,1;
                #my $string;
                printf $out_usr_fn "%s",$tmp_remove[0];
                @tmp_remove2=splice @usr_output_arg_list,0,1;
               # sprintf($string,"\tmemcpy(%s,%s,sizeof(*%s));\n",$tmp_remove2[0],$tmp_remove[0],sizeof(*$tmp_remove[0]))
                #printf "tmp_remove:$tmp_remove[0] tmp_remove2:$tmp_remove[0]";
                push @usr_output_rem_code2,"\tmemcpy($tmp_remove2[0],$tmp_remove[0],sizeof(*$tmp_remove[0]));\n";
            }
            else#in only
            {
                @tmp_remove=splice @usr_input_arg_list,0,1;
                printf $out_usr_fn "%s",$tmp_remove[0];
                next;  
            }
        }      
        }
        printf $out_usr_fn ");\n";
        foreach my $index(0 .. $#usr_output_rem_code2)
        {
             printf $out_usr_fn "%s",$usr_output_rem_code2[$index];
        }        
        foreach my $index(0 .. $#usr_output_rem_code)
        {
             printf $out_usr_fn "%s",$usr_output_rem_code[$index];
        }
        printf $out_usr_fn "}\n";
    }
                    
    }
   }
    my $filename="ANON";
    if($h_file=~m/.*\/(.+)\.\w/)
    {
        $filename=$1;
    }
    else
    {
        if($h_file=~m/(.+)\.\w/)
        {
            $filename=$1;            
        }
    }
    printf $out_usr_fn "int %s_kcu_register(void)\n{\n",$filename;
    printf $out_usr_fn "\tint ret=0;\n";    
    foreach(@register_list)
    {
        printf $out_usr_fn "\tret=%s;\n",$_; 
        print $out_usr_fn "\tif(ret)return ret;\n";
    }
    printf $out_usr_fn "\treturn ret;\n}\n"; 
    printf "\nplease add below nfy id into the definition of enum NFY_USR_ID under linux/soc/mediatek/kcu.h:\n\n";
    foreach(@nfy_id_list)    
    {
        print "    $_,\n";
    }
    print "\n";
    close($out_usr_fn);
    close($out_fn);
    close($input_h_fn);
    return @struct_def;
}

sub add_struct_define
{
    (my $input_file,my @arg_def)=@_;
    my @content;
    my $lastline="";
    
    open(my $input_file_fn,"<$input_file") || die "can not open $input_file for read\n";
    @content=<$input_file_fn>;
    close($input_file_fn);
    while($lastline=~m/^\s*$/)
    {
        $lastline=pop @content;
    }
    foreach(@arg_def) #insert struct definition
    {
        push @content,$_;
    }
    push @content,$lastline;
    
    open($input_file_fn,">$input_file") || die "can not open $input_file for write\n";
    foreach(@content)
    {
        printf $input_file_fn "$_";
    }
    close($input_file_fn);
}

if($#ARGV<2)
{
    print "usage:\n\tconstruct_kcu_api.pl input_h_file ouput_c_file output_usr_c_file\n";
    exit;
}
my @struct_arg_def=construct_api(@ARGV);

add_struct_define($ARGV[0],@struct_arg_def);

