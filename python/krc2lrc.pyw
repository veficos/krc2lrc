# coding=utf-8
# author=veficos

import codecs
import zlib
import re
import os

from tkinter import *
from tkinter import ttk
from tkinter.filedialog import askopenfilename, askdirectory


def decompress_krc(krcbytes):
    key = bytearray([ 0x0040, 0x0047, 0x0061, 0x0077, 0x005e, 0x0032, 0x0074, 0x0047, 0x0051, 0x0036, 0x0031, 0x002d, 0x00ce, 0x00d2, 0x006e, 0x0069])
    decompress_bytes = []
    i = 0

    for ch in krcbytes[4:]:
        decompress_bytes.append(ch ^ key[i % 16])
        i = i + 1

    decode_bytes = zlib.decompress(bytearray(decompress_bytes)).decode('utf-8-sig')
    decode_bytes = re.sub(r'<[^>]*>', '', decode_bytes)

    for match in re.finditer(r'\[(\d*),\d*\]', decode_bytes):
        ms = int(match.group(1))
        time = '[%.2d:%.2d.%.2d]' % ((ms % (1000 * 60 * 60)) / (1000 * 60), (ms % (1000 * 60)) / 1000, (ms % (1000 * 60)) % 100)
        decode_bytes = decode_bytes.replace(match.group(0), time)

    return decode_bytes


def krc2lrc(file, saveto):
    with codecs.open(file, 'rb') as f:
        decode_bytes = decompress_krc(bytearray(f.read()))
        fp = codecs.open(saveto, "w", 'utf-8')
        fp.write(decode_bytes)
        fp.close()


root = Tk()
savedir = ''

def select_files_command():
    names = askopenfilename(filetypes =(("Kugou Krc File", "*.krc"), ("All Files", "*.*")),
                            title = "Choose a krc file.", multiple=True)
    
    try:
        for file in names:
            saveto = os.path.join(savedir, os.path.basename(file).replace('.krc', '.lrc') if savedir else file.replace('.krc', '.lrc'))
            krc2lrc(file, saveto)
            label = ttk.Label(root, text ="%s转换成功" % os.path.basename(file))
            label.pack(side=BOTTOM)
    except Exception as e:
        label = ttk.Label(root, text ="转换失败: %s" % str(e))
        label.pack(side=BOTTOM)

def select_savedir_command():
    global savedir
    
    name = askdirectory()
    savedir = os.path.join(savedir, name)
    

Title = root.title("krc2lrc小工具")

menu = Menu(root)
root.config(menu=menu)

file = Menu(menu)

file.add_command(label = 'Open', command = select_files_command)
file.add_command(label = 'SaveTo', command = select_savedir_command)

menu.add_cascade(label = 'File', menu = file)
menu.add_cascade(label = 'Exit', command = lambda:exit())

root.mainloop()
