const path = require("path");
const HtmlWebpackPlugin = require("html-webpack-plugin");
const TsconfigPathsPlugin = require("tsconfig-paths-webpack-plugin");

module.exports = env => {
    return {
        entry: "./index.ts",
        devtool: "inline-source-map",
        context: __dirname,
        watch: false,
        module: {
            rules: [
                {
                    test: /\.tsx?$/,
                    use: "ts-loader",
                    exclude: /node_modules/,
                },
            ],
        },
        resolve: {
            extensions: [".ts", ".js"],
            plugins: [
                new TsconfigPathsPlugin({}),
            ],
        },
        output: {
            filename: env.moduleName ? env.moduleName + ".js" : "bundle.js",
            path: env.outPath ? env.outPath : path.resolve(__dirname, "../../../build/js"),
        },
        plugins: [
            new HtmlWebpackPlugin({
                template: "./index.ejs",
                templateParameters: {
                    moduleName: env.moduleName ? env.moduleName : "harvest",
                },
            }),
        ]
    };
};
