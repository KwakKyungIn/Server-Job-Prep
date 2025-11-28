import argparse
import jinja2
import ProtoParser
import os

def main():
    arg_parser = argparse.ArgumentParser(description = 'PacketGenerator')
    arg_parser.add_argument('--path', type=str, default='Protocol.proto', help='proto path')
    arg_parser.add_argument('--output', type=str, default='TestPacketHandler', help='output file')
    arg_parser.add_argument('--recv', type=str, default='C_', help='recv convention')
    arg_parser.add_argument('--send', type=str, default='S_', help='send convention')
    arg_parser.add_argument('--id', type=int, default=1000, help='start packet id')
    # [GIGACHAD MODIFIED] 언어 선택 옵션 추가 (기본값 cpp)
    arg_parser.add_argument('--lang', type=str, default='cpp', help='language (cpp/csharp)')
    args = arg_parser.parse_args()

    parser = ProtoParser.ProtoParser(args.id, args.recv, args.send)
    parser.parse_proto(args.path)

    # 경로에서 파일명만 추출
    file_name = os.path.splitext(os.path.basename(args.path))[0]

    file_loader = jinja2.FileSystemLoader('Templates')
    env = jinja2.Environment(loader=file_loader)

    # [GIGACHAD MODIFIED] 언어에 따라 템플릿과 확장자 결정
    if args.lang == 'csharp':
        template = env.get_template('PacketManager.cs') # C#용 템플릿 (이제 만들거임)
        output_file_name = args.output + '.cs'
    else:
        template = env.get_template('PacketHandler.h')  # 기존 C++ 템플릿
        output_file_name = args.output + '.h'
    
    # 템플릿 렌더링
    output = template.render(parser=parser, output=args.output, proto_file=file_name)

    # [GIGACHAD MODIFIED] 결정된 파일명으로 저장
    f = open(output_file_name, 'w+')
    f.write(output)
    f.close()

    print(f"[SUCCESS] Generated {output_file_name}")
    return

if __name__ == '__main__':
    main()