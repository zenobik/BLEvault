from Crypto.Cipher import AES
import csv

key = 'abcdefghijkl'
pin = '0000'
cipher = AES.new(key+pin, AES.MODE_ECB)
f = open('passwords', 'w')
txt_buffer=''
name_truncated = False
pass_truncated = False
with open('pass.csv') as csvfile:
    row_count = sum(1 for row in csvfile)
    buf = '{:<16}'.format(str(row_count).rjust(3, '0'))
    buf += cipher.encrypt('{:<32}'.format('OK')).encode('hex')
    buf += '\n'
    txt_buffer += buf

with open('pass.csv') as csvfile:
    reader = csv.reader(csvfile, dialect='excel', delimiter=';', quotechar='"')
    for row in reader:
        if len(row[0])>16:
            name_truncated=True
        if len(row[1])>32:
            pass_truncated=True
        new_row = '{:<16}'.format(row[0][:16])
        new_row += cipher.encrypt('{:<32}'.format(row[1][:32])).encode('hex')
        new_row += '\n'
        txt_buffer += new_row

if name_truncated==True:
    print('Warning: some names have been truncated because length was over the 16 chars limit')
if pass_truncated==True:
    print('ERROR: some PASSWORDS have been truncated because length was over the 32 chars limit')

f.write(txt_buffer)
f.close()
print('DONE!')


 

 

