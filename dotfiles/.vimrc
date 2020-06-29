"set wildmenu
""set wildmode=list:longest,full
"set wildmode=list,full
"set wildignore+=.*
"set path=**,/usr/include/**

set hlsearch ignorecase incsearch smartcase

set tabstop=4 expandtab shiftwidth=4 smarttab
set autoindent cindent
"set number

filetype plugin indent on
syntax on
"colorscheme desert

set hidden "allow swithcing buffers before saving
set showcmd

let mapleader=" "
"nnoremap <leader>f :bro ol<CR>
nnoremap F :bro ol<CR>
nnoremap <leader>b :ls<CR>:buffer<space>|

nnoremap <S-b> :bn<CR>
nnoremap <S-t> gt
nnoremap <S-w> <C-W><C-W> 
"ctrl-w ctrl-w    - move cursor to another window (cycle)
"ctrl-w up arrow  - move cursor up a window
":tab sball //open all buffers in tab
"jump ctrl-o/i
"oldfile / browse oldfile
nnoremap <leader>s :mks! ~/vim-sessions/last.vim<cr>
nnoremap <leader>r :so ~/vim-sessions/last.vim<cr>

nnoremap <C-L> :nohlsearch<CR><C-L>

set foldmethod=syntax
set nofoldenable
set tags=./tags;
"set mouse=a

set rtp+=~/.fzf
set rtp+=~/.vim/bundle/fzf.vim

