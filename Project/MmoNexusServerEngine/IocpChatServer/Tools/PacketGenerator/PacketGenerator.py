import argparse
import jinja2
import ProtoParser
import os  # [추가] 파일명 추출용

def main():
    arg_parser = argparse.ArgumentParser(description = 'PacketGenerator')
    arg_parser.add_argument('--path', type=str, default='Protocol.proto', help='proto path')
    arg_parser.add_argument('--output', type=str, default='TestPacketHandler', help='output file')
    arg_parser.add_argument('--recv', type=str, default='C_', help='recv convention')
    arg_parser.add_argument('--send', type=str, default='S_', help='send convention')
    arg_parser.add_argument('--id', type=int, default=1000, help='start packet id')
    args = arg_parser.parse_args()

    parser = ProtoParser.ProtoParser(args.id, args.recv, args.send)
    parser.parse_proto(args.path)

    # [추가] 경로에서 파일명만 추출 (예: "./Protocol_S2S.proto" -> "Protocol_S2S")
    file_name = os.path.splitext(os.path.basename(args.path))[0]

    file_loader = jinja2.FileSystemLoader('Templates')
    env = jinja2.Environment(loader=file_loader)

    template = env.get_template('PacketHandler.h')
    
    # [수정] 템플릿에 proto_file 인자 전달
    output = template.render(parser=parser, output=args.output, proto_file=file_name)

    f = open(args.output+'.h', 'w+')
    f.write(output)
    f.close()

    print(output)
    return

if __name__ == '__main__':
    main()