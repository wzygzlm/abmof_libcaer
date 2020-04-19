import sys
import os
import ctypes

POLARITY_SHIFT = 11
POLARITY_MASK = (1 << POLARITY_SHIFT)
POLARITY_Y_ADDR_SHIFT = 22
POLARITY_Y_ADDR_MASK = (511 << POLARITY_Y_ADDR_SHIFT)
POLARITY_X_ADDR_SHIFT = 12
POLARITY_X_ADDR_MASK = (1023 << POLARITY_X_ADDR_SHIFT)

def main():
   filepath = sys.argv[1]

   if not os.path.isfile(filepath):
       print("File path {} does not exist. Exiting...".format(filepath))
       sys.exit()

   bag_of_words = {}
   with open(filepath) as fp:
       cnt = 0
       f = open(filepath+'_bin', 'w+b')

       for line in fp:
           print("processing line {}".format(cnt))
           if cnt <= 2:
               print("line {} contents {}".format(cnt, line))
           else:
               lineList = list(map(int, line.split()))
               ts = lineList[0]
               x = lineList[1]
               y = lineList[2]
               pol = lineList[3]
               OF_scale = lineList[6]
               OF_x = ((lineList[4]) >> OF_scale)
               if OF_x < 0:
                   OF_x = -OF_x + 4
               OF_y = ((lineList[5]) >> OF_scale)
               if OF_y < 0:
                   OF_y = -OF_y + 4
               rotateFlg = lineList[7]
               OF_ret = (rotateFlg << 8) + (OF_scale << 6) + (OF_y << 3) + OF_x

               address = (y << POLARITY_Y_ADDR_SHIFT) + (x << POLARITY_X_ADDR_SHIFT)+ (pol << POLARITY_SHIFT) + OF_ret
               addr_arr = address.to_bytes(4, 'big')
               addr_bin = bytearray(addr_arr)
               ts_arr = ts.to_bytes(4, 'big')
               ts_bin = bytearray(ts_arr)
               f.write(addr_bin)
               f.write(ts_bin)
               # print("line {} ts {:02x} address {:02x} OF_x {} OF_y {}".format(cnt, ts, address, OF_x, OF_y))
               # print(byte_arr)
           cnt += 1
   f.close()
   print("Convert finished.")

def order_bag_of_words(bag_of_words, desc=False):
   words = [(word, cnt) for word, cnt in bag_of_words.items()]
   return sorted(words, key=lambda x: x[1], reverse=desc)

def record_word_cnt(words, bag_of_words):
    for word in words:
        if word != '':
            if word.lower() in bag_of_words:
                bag_of_words[word.lower()] += 1
            else:
                bag_of_words[word.lower()] = 1

if __name__ == '__main__':
    main()
