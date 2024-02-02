

# 485_device_gateway

485设备网关，以485端口为单位，每个端口可分别作TCP、UDP服务端或客户端<br>
启用http服务可以实现远程配置<br>
端口设置相关API已嵌入代码，前端自行实现<br><br>

<!-- PROJECT SHIELDS -->

<img src="https://img.shields.io/badge/platform-linux-green"> <img src="https://img.shields.io/badge/host-arm linux-purple"> <img src="https://img.shields.io/badge/language-C-yellow"> <img src="https://img.shields.io/badge/version-v0.0.2-blue"> 

<!-- PROJECT LOGO -->
<br />

<p align="center">


  <p align="center">
    <a href="https://github.com/koudaiNEW/485_device_gateway">查看Demo</a>
    ·
    <a href="https://github.com/koudaiNEW/485_device_gateway/issues">报告Bug</a>
    ·
    <a href="https://github.com/koudaiNEW/485_device_gateway/issues">提出新特性</a>
  </p>

</p>
 
## 目录

- [克隆仓库](#克隆仓库)
- [文件目录说明](#文件目录说明)
- [使用到的框架与库](#使用到的框架与库)
- [版本控制](#版本控制)
- [鸣谢](#鸣谢)

### 克隆仓库

```sh
git clone https://github.com/koudaiNEW/485_device_gateway.git
```

### 文件目录说明
eg:

```
filetree 
├── LICENSE
├── README.md
├── API.xlsx
├── /src/
│  ├── Makefile
│  ├── cJSON.c
│  ├── cJSON.h
│  ├── client.json
│  ├── http_parser.c
│  ├── http_parser.h
│  ├── http_server.c
│  ├── http_server.h
│  ├── user_config.c
│  ├── user_config.h
│  ├── uv_server.c
└──└── uv_server.h
```

### 使用到的框架与库

- [libuv](https://docs.libuv.org/en/v1.x/)
- [http_parser](https://github.com/nodejs/http-parser/tree/main)
- [cJSON](https://github.com/DaveGamble/cJSON)

### 版本控制

该项目使用Git进行版本管理。您可以在repository参看当前可用版本。

### 版权说明

该项目签署了MIT 授权许可，详情请参阅 [LICENSE](https://github.com/koudaiNEW/485_device_gateway/blob/main/LICENSE)

### 鸣谢


- [GitHub Emoji Cheat Sheet](https://www.webpagefx.com/tools/emoji-cheat-sheet)
- [Img Shields](https://shields.io)
- [Choose an Open Source License](https://choosealicense.com)
- [GitHub Pages](https://pages.github.com)
- [Animate.css](https://daneden.github.io/animate.css)
- [Libuv](https://github.com/libuv/libuv)
- [http_parser](https://github.com/nodejs/http-parser?tab=readme-ov-file)
- [cJSON](https://github.com/DaveGamble/cJSON)

<!-- links -->
