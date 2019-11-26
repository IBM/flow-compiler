"copy this file into ~/.vim/syntax
"add the following line to the end of .vimrc
"au BufRead,BufNewFile *.flow          set filetype=flow

" quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

syn case match

syn keyword protoTodo       contained TODO FIXME XXX
syn cluster protoCommentGrp contains=protoTodo

syn keyword protoSyntax     syntax import option
syn keyword protoStructure  package message group
syn keyword protoRepeat     optional required repeated
syn keyword protoDefault    default
syn keyword protoExtend     extend extensions to max
syn keyword protoRPC        service rpc returns
syn keyword flowStructure   node entry 
syn keyword flowStatement   return output error 
syn keyword flowField       image pod port environment mount replicas limits init endpoint

syn keyword protoType      int32 int64 uint32 uint64 sint32 sint64
syn keyword protoType      fixed32 fixed64 sfixed32 sfixed64
syn keyword protoType      float double bool string bytes
syn keyword protoTypedef   enum
syn keyword protoBool      true false

syn match   protoInt     /-\?\<\d\+\>/
syn match   protoInt     /\<0[xX]\x+\>/
syn match   protoFloat   /\<-\?\d*\(\.\d*\)\?/
syn match   protoStructure /[A-Za-z][A-Za-z0-9_]*[@!~$#%&^]/
syn match   flowInternal /[~][A-Za-z][A-Za-z0-9]*/
syn region  protoComment start="\/\*" end="\*\/" contains=@protoCommentGrp
syn region  protoComment start="//" skip="\\$" end="$" keepend contains=@protoCommentGrp
syn region  protoString  start=/"/ skip=/\\./ end=/"/
syn region  protoString  start=/'/ skip=/\\./ end=/'/

hi def link protoTodo         Todo

hi def link protoSyntax       Include
hi def link protoStructure    Structure
hi def link flowStructure     Structure
hi def link flowStatement     Statement
hi def link flowField         PreProc
hi def link protoRepeat       Repeat
hi def link protoDefault      Keyword
hi def link protoExtend       Keyword
hi def link protoRPC          Keyword
hi def link protoType         Type
hi def link protoTypedef      Typedef
hi def link flowInternal      Function
hi def link protoBool         Boolean

hi def link protoInt          Number
hi def link protoFloat        Float
hi def link protoComment      Comment
hi def link protoString       String

let b:current_syntax = "flow"
