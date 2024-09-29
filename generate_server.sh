docker run --rm -u $(id -u):$(id -g) -v "${PWD}:/local" openapitools/openapi-generator-cli generate -i /local/openapi.yaml -g cpp-pistache-server -o /local/openapi/

git apply *.patch