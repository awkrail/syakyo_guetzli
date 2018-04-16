# ソースコード
- guetzli.cc(main)
  - processor.h .. 一連の処理を行うクラスやメソッドが入っている(重要)
  - quality.h .. guetzli処理を行う上でのqualityの調整
  - jpeg_data_reader.h .. JPEGデータを読み込む時の関数が入っている。PNGはlibpngに依存
  - jpeg_data.h .. 内部で保持するデータ構造など(重要)
  - stats.h .. デバッグ時に標準出力する時など
 
 # Tree
 - guetzli.cc
  - processor.h
    - comparator.h
      - output_image.h
        - jpeg_data.h
      - stats.h
    - jpeg_data.h
    - stats.h
  - quality.h
  - jpeg_data_reader.h
    - jpeg_data.h
  - jpeg_data.h
    - jpeg_error.h
  - stats.h