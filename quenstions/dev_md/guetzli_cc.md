# 全体
- ソースコードは作るたびにpremake打たないとダメ

# main
- strnlenは, 与えられた文字列(char*)とmaxの大きさを取り, max以下ならばその文字数を返し, max以上ならばmaxを返す

# ReadFileOrDie
- strncmpは二つの文字列を比較する。第三引数に文字数を入れる
- fseekについて
  - fseekはファイルポインタを指定の位置まで移動させる
    - SEEK_SET : ファイルの最初
    - SEEK_CUR : 与えられたところまでポインタを写す
    - SEEK_END : 終わりまで写す
    - 返り値は成功した時に0を返す
  - fseekで最後までシークさせてftellと組み合わせることでbuffer sizeを取得することができる
  - fread : 引数は 代入したいポインタ, データのサイズ(型), データの大きさ, filestream
- std::unique_ptrについて(スマートポインタ)
  - https://qiita.com/hmito/items/db3b14917120b285112f
  - メモリの解放忘れや不適切な対象への delete 実行をなくしてくれるもの
  - 確保したメモリを自動的に開放してくれるようにした
  - unique_ptrの特徴
    - あるメモリの所有権を持つ unique_ptr<T> はただ一つのみである
    - コピーができない
    - 通常のポインタに匹敵する速度
    - 配列を扱える
    - deleterを指定できる　=> ある関数を介して確保/解放しなければならない場合
- std::shared_ptrというのもあるらしい
  - 同一のメモリの所有権を複数で共有できるようにしたもの
  - あるメモリの所有権を複数の shared_ptr<T> が所有することができる
  - メモリの解放は全ての所有権を持つ shared_ptr<T> が破棄された時に実行される
  - コピー/ムーブが可能

# ReadPNG
- libpngが出てくるところ
  1. png_create_read_struct, png_create_info_structで必要な構造体を作る
  2. setjmpを使ってpng_create_read_structのエラーを補足する必要がある(https://www.hiroom2.com/2014/05/28/libpng%E3%81%AEc-%E3%82%A4%E3%83%B3%E3%82%BF%E3%83%BC%E3%83%95%E3%82%A7%E3%83%BC%E3%82%B9%E3%81%A7%E3%81%82%E3%82%8Bpng-%E3%82%92%E4%BD%BF%E3%81%A3%E3%81%A6%E3%81%BF%E3%82%8B/)
- png_set_read_fnの部分
  - png_set_read_fnはユーザが定義した関数でpngを読み込む
  - png_ptrで入力となる, 上のpng_ptrを与える
  - 第二引数はユーザが定義した構造体
  - 第三引数が関数 : 

# あとで読む
- libpngのマニュアル
- http://www.libpng.org/pub/png/libpng-1.2.5-manual.html