set wildmenu
"set wildmode=list:longest,full
set wildmode=list,full
set wildignore+=.*
set path=**

set term=xterm-256color
set hlsearch ignorecase incsearch smartcase

set tabstop=4 expandtab shiftwidth=4 smarttab
set autoindent cindent
set number

filetype plugin indent on
syntax on
colorscheme pablo

set hidden "allow swithcing buffers before saving
set showcmd

let g:netrw_banner=0
let g:netrw_winsize=20
let g:netrw_liststyle=3
"toggle netrw on the left side of the editor with \n
nnoremap <leader>f :Lexplore<CR>

nnoremap <S-b> :bn<CR>
nnoremap <S-t> gt
nnoremap <S-w> <C-W><C-W> 
"ctrl-w ctrl-w    - move cursor to another window (cycle)
"ctrl-w up arrow  - move cursor up a window
":tab sball //open all buffers in tab

nnoremap <C-L> :nohlsearch<CR><C-L>

set foldmethod=syntax
