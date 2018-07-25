"This is a comment
"
".vimrc - a basic file for newbies to start with
"

:filetype on
:au FileType c,cpp,java set cindent

" Uncomment this if you use a dark background on your terminal
highlight Normal ctermfg=grey ctermbg=darkblue
set bg=dark
set nocompatible
	" These next 2 should have the same value
set tabstop=3
set shiftwidth=3
set showmode
set showmatch
set pastetoggle=<F2>
set textwidth=100 "76
set backspace=2
	" Highlight search.  Comment out if that really annoys you.
:autocmd GUIEnter * call libcallnr("gvimfullscreen.dll", "ToggleFullScreen", 0)
colorscheme slate
set hlsearch
set ruler
set number
if &t_Co > 1
	syntax enable
endif
if has("gui_running")
  if has("gui_gtk2")
    set guifont=Inconsolata\ 12
  elseif has("gui_macvim")
    set guifont=Menlo\ Regular:h14
  elseif has("gui_win32")
    set guifont=Consolas:h12:cANSI
  endif
endif
syntax on
