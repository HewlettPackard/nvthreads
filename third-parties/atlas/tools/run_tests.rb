# Keep in sync with the makefile targets
targets = ['all', 'use-table-flush', 'use-movnt', 'flc', 'fgc', \
           'flc-stats', 'fgc-stats', 'stats', \
           'disable-flush', 'disable-flush-stats', \
           'flc-disable-flush', 'fgc-disable-flush', \
           'flc-disable-flush-stats', \
          ]
test_array = [
              'queue_orig', 'queue_nvm', \
              'cow_array_list', 'cow_array_list_nvm', \
              'sll', 'sll_ll', 'sll_nvm', \
              'tester', \
             ]
nvm_test_array = [
                  'queue_nvm', 'cow_array_list_nvm', \
                  'sll_nvm', 'tester' \
                 ]

$ruby_version_int = nil

#### Sets global variable with version number ####
def get_version(fp, logfile)
  print_msg("Ruby version " + RUBY_VERSION, fp, logfile)
  $ruby_version_int = RUBY_VERSION[2].to_i
end

#### Print a message onto stdout and a provided file ####
def print_msg(msg, fp, logfile)
  puts msg
  fp.puts msg
  headline = "echo \"#### " + msg + "\" >> " + logfile
  system(headline)
end

#### Make a target ####
def make_target(target_name, logfile, fp)
  headline = "echo \"#### make clean " + target_name + "\" >> " + logfile
  system(headline)

  make_cmd = 'make clean ' + target_name + \
             ' >> ' + logfile + ' 2>> ' + logfile
  status_make = system(make_cmd)
  if (!status_make)
    print_msg("---- Making failed ----", fp, logfile)
    return false
  else
    return true
  end
end

#### Make recovery target ####
def make_recovery(target_name, logfile, fp)
  headline = "echo \"#### make clean force-fail\" >> " + logfile
  system(headline)

  make_cmd = 'make clean force-fail >> ' + logfile + \
             ' 2>> ' + logfile
  status_make = system(make_cmd)
  if (!status_make)
    print_msg("---- Making failed ----", fp, logfile)
    return false
  else
    return true
  end
end

#### Populate an array with the lines in the provided file ####
def populate_array_from_file(filename)
  lines_arr = []
  File.open(filename).each {|line| lines_arr << line }
  return lines_arr
end

#### Verify whether the first file is contained in the second ####
def is_file_contained(file1, file2)
  file1_lines = populate_array_from_file(file1)
  file2_lines = populate_array_from_file(file2)
  result_lines = file1_lines - file2_lines
  return result_lines.empty?
end

#### Extract timing from output file ####
def get_timing_from_output_file(out_file)
  File.open(out_file).each do |line|
   tmp_str = String.new(line)
   if (tmp_str.include?"time elapsed")
     tmp_arr = []
     if $ruby_version_int > 8 then
       tmp_str.split(" ") do |one_str|
         tmp_arr << one_str
       end
     else
       tmp_str.each(separator=" ") do |one_str|
         tmp_arr << one_str
       end
     end
     return tmp_arr.at(2).to_i
   end
  end
  return 2**30-1
end

#### Initialize timing information ####
def init_timing_info(timing_file, tracking_hash, nvm_hash)
  timing_fp = File.open(timing_file)
  timing_fp.each do |timing_line|
    timing_str = String.new(timing_line)
    if (timing_str.include?"#") then
      next
    end
    tmp_arr = []
    if $ruby_version_int > 8 then
      timing_str.split(" ") do |one_str|
        tmp_arr << one_str.chop
      end
    else
      timing_str.each(separator=" ") do |one_str|
        tmp_arr << one_str.chop
      end
    end
    tracking_hash[tmp_arr.at(0)] = 0
    tracking_hash[tmp_arr.at(1)] = 0
    tmp_val = []
    tmp_val << tmp_arr.at(0)
    tmp_val << tmp_arr.at(2)
    tmp_val << tmp_arr.at(3)
    nvm_hash[tmp_arr.at(1)] = tmp_val
  end
  timing_fp.close
end

#### Update the hash table with timing information ####
def update_runtime_info(all_ver, test_name, runtime)
  if (all_ver.has_key?(test_name))
    all_ver[test_name] = all_ver.fetch(test_name) + runtime
  end
end

#### Run a crash test once ####
#### This is expected to fail, so we don't capture output/status
def run_crash_test_once(test_name, target_name, test_dir, logfile, fp)
  test_in = test_dir + test_name + ".in"
  test_cmd = test_dir + test_name + \
             " `cat " + test_in + "`" + '> /dev/null 2>&1'

  headline = "echo \"#### " + test_cmd + "\" >> " + logfile
  system(headline)

  status_run = system(test_cmd)
  return true
end

#### Run a test twice ####
def run_test_twice(test_name, test_dir, target_name, logfile, all_ver, phase, fp)
  test_in = test_dir + test_name + ".in"
  test_ref = test_dir + test_name + ".ref"
  test_out = test_dir + test_name + "." + target_name + \
             "." + (2*phase).to_s + '.out'
  test_cmd = test_dir + test_name + \
             " `cat " + test_in + "`" + '>> ' + test_out + ' 2>&1'

  headline = "echo \"#### " + test_cmd + "\" >> " + logfile
  system(headline)

  status_run1 = system(test_cmd)
  cat_cmd = 'cat ' + test_out + ' >> ' + logfile
  system(cat_cmd)
  runtime = get_timing_from_output_file(test_out)
  if (!status_run1 || !is_file_contained(test_ref, test_out))
    msg = test_name + " failed (Compare " + test_ref + " and " + test_out + ")"
    print_msg(msg, fp, logfile)
    return false
  end
  rm_cmd = "/bin/rm -f " + test_out
  system(rm_cmd)
  test_out = test_dir + test_name + "." + target_name + \
             "." + (2*phase+1).to_s + '.out'
  test_cmd = test_dir + test_name + \
             " `cat " + test_in + "`" + '>> ' + test_out + ' 2>&1'

  headline = "echo \"#### " + test_cmd + "\" >> " + logfile
  system(headline)

  status_run2 = system(test_cmd)
  cat_cmd = 'cat ' + test_out + ' >> ' + logfile
  system(cat_cmd)
  runtime += get_timing_from_output_file(test_out)
  update_runtime_info(all_ver, test_name, runtime)
  if (!status_run2 || !is_file_contained(test_ref, test_out))
    msg = test_name + " failed (Compare " + test_ref + " and " + test_out + ")"
    print_msg(msg, fp, logfile)
    return false
  end
  rm_cmd = "/bin/rm -f " + test_out
  system(rm_cmd)
  return true
end

#### Run recovery for a test ####
def run_recovery(test_name, logfile, fp)
  recover_cmd = './tools/recover ' + test_name + ' >> ' + logfile + \
                ' 2>> ' + logfile

  headline = "echo \"#### ./tools/recover " + test_name + "\" >>" + logfile
  system(headline)

  recover_status = system(recover_cmd)
  if (!recover_status)
     return false
  else
    return true
  end
end  

#### Main body ####

if (ARGV.size != 1)
  abort "Error: specify a mode"
end
quick_test = false
if (ARGV[0].match("quick"))
  quick_test = true
end
num_passed_tests = 0
num_failed_tests = 0
num_perf_regression = 0

atlas_dir = "./"
test_dir = atlas_dir + "tests/"
logfile = test_dir + "log"
clean_cmd = '/bin/rm -f ' + logfile + ' ' + test_dir + "*.out"
summary_file = test_dir + "summary.txt"
timing_file = test_dir + "timing.txt"

system(clean_cmd)

all_ver = Hash.new
nvm_ver = Hash.new

fp = File.open(summary_file, 'w')

get_version(fp, logfile)
print_msg("Cleaning memory", fp, logfile)

headline = "echo \"#### make clean_memory\" >> " + logfile
system(headline)

make_cmd = 'make clean_memory >> ' + logfile
system(make_cmd)

targets.each do |target_name|
  init_timing_info(timing_file, all_ver, nvm_ver)
  print_msg("#################################", fp, logfile)
  print_make_cmd = '---- Making and testing target ' + target_name + ' ----'
  print_msg(print_make_cmd, fp, logfile)
  status_make = make_target(target_name, logfile, fp)
  if (!status_make)
    num_failed_tests += test_array.length
  else
    test_array.each do |test_name|
      status_recover = run_recovery(test_name, logfile, fp)
      status_run = run_test_twice(\
                     test_name, test_dir, target_name, logfile, all_ver, 0, fp)
      if (!status_recover || !status_run)
        num_failed_tests += 1
      else
        msg = test_name + ' passed'
        print_msg(msg, fp, logfile)
        num_passed_tests += 1
      end
    end
    print_msg("---- Performing crash/recovery testing ----", fp, logfile)
    status_make = make_recovery(target_name, logfile, fp)
    if (!status_make)
      num_failed_tests += test_array.length
    else
      nvm_test_array.each do |test_name|
        status_run = run_crash_test_once(test_name, target_name, test_dir, logfile, fp)
        if (status_run)
          status_recover = run_recovery(test_name, logfile, fp)
          if (!status_recover)
            msg = test_name + " failed crash/recovery"
            print_msg(msg, fp, logfile)
            num_failed_tests += 1
          else
            msg = test_name + " passed crash/recovery"
            print_msg(msg, fp, logfile)
            num_passed_tests += 1
          end
        else
          print_msg("---- Skipping recovery since crash testing failed ----", fp, logfile)
          num_failed_tests += 1
        end
      end
    end
  end
  print_msg("---- Retesting after crash/recovery testing ----", fp, logfile)
  status_make = make_target(target_name, logfile, fp)
  if (!status_make)
    num_failed_tests += test_array.length
  else
    test_array.each do |test_name|
      status_run = run_test_twice(\
                     test_name, test_dir, target_name, logfile, all_ver, 1, fp)
      if (!status_run)
        num_failed_tests += 1
      else
        msg = test_name + ' passed'
        print_msg(msg, fp, logfile)
        num_passed_tests += 1
      end
    end
  end
  tn = target_name.chomp
  if (tn == "all" || tn == "flc" || tn == "fgc" || \
      tn == "use-table-flush" || tn == "use-movnt")
    nvm_ver.each do |key, value|
      timing_info_nvm = all_ver.fetch(key)
      timing_info_orig = all_ver.fetch(value.at(0))
      if (timing_info_orig != 0)
        ratio = timing_info_nvm/timing_info_orig
        if (ratio < value.at(1).to_i || ratio > value.at(2).to_i)
          print_msg("Possible performance regression for #{key}, (expected ratio between #{value.at(1)} & #{value.at(2)}, but found #{ratio})", fp, logfile)
          num_perf_regression += 1
        end
      else
        puts "Warning: Base timing is 0, counting as performance regression"
        num_perf_regression += 1
      end
    end
  end
  all_ver.clear
  nvm_ver.clear
  if (quick_test == true)
    break
  end
end
make_cmd = "make clean > /dev/null 2>&1"
system(make_cmd)
print_msg("-----------------------", fp, logfile)

total_tests = num_passed_tests + num_failed_tests
print_msg("Total number of tests: #{total_tests}", fp, logfile)
print_msg("Number of tests passed: #{num_passed_tests}", fp, logfile)
print_msg("Number of tests failed: #{num_failed_tests}", fp, logfile)
print_msg("Number of possible performance regressions: #{num_perf_regression}", fp, logfile)
print_msg("-----------------------", fp, logfile)
print_msg("See summary.txt, log, and *.out files in #{test_dir} for further details", fp, logfile)

fp.close
