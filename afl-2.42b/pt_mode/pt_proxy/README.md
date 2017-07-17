==========================
american fuzzy lop-PT mode
==========================

  Written and maintained by Yaohui Chen <yaohway@gmail.com> and Jun Xu <junxzm@hotmail.com>
  
To prepare pt fuzzing, the following setup steps are required:
1) download glibc and patch fork_server before compilation
2) download elfpatcher and patch the interpreter of target COTS binary
3) compile pt module
4) compile proxy
5) launch AFL with additional flag "-P" 

