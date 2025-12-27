library('JenkinsPipelineUtils') _

withCredentials([
    string(credentialsId: 'WIFI_PASSWORD', variable: 'WIFI_PASSWORD'),
]) {
    podTemplate(inheritFrom: 'jenkins-agent-large', containers: [
        containerTemplate(name: 'idf', image: 'espressif/idf:v5.5.2', command: 'sleep', args: 'infinity', envVars: [
            containerEnvVar(key: 'WIFI_PASSWORD', value: '$WIFI_PASSWORD'),
        ])
    ]) {
        node(POD_LABEL) {
            stage('Build underfloor heating controller') {
                dir('esp-libs') {
                    git branch: 'main',
                        credentialsId: '5f6fbd66-b41c-405f-b107-85ba6fd97f10',
                        url: 'https://github.com/pvginkel/esp-libs.git'
                }

                dir('UnderfloorHeatingController') {
                    git branch: 'main',
                        credentialsId: '5f6fbd66-b41c-405f-b107-85ba6fd97f10',
                        url: 'https://github.com/pvginkel/UnderfloorHeatingController.git'
                        
                    container('idf') {
                        // Necessary because the IDF container doesn't have support
                        // for setting the uid/gid.
                        sh 'git config --global --add safe.directory \'*\''

                        sh '/opt/esp/entrypoint.sh idf.py build'
                    }
                }
            }

            stage('Deploy underfloor heating controller') {
                dir('HelmCharts') {
                    git branch: 'main',
                        credentialsId: '5f6fbd66-b41c-405f-b107-85ba6fd97f10',
                        url: 'https://github.com/pvginkel/HelmCharts.git'
                }

                dir('UnderfloorHeatingController') {
                    sh 'cp build/underfloor-heating-controller.bin underfloor-heating-controller-ota.bin'

                    sh 'chmod +x scripts/upload.sh'
                    sh 'scripts/upload.sh ../HelmCharts/assets/kubernetes-signing-key underfloor-heating-controller-ota.bin'
                }
            }
        }
    }
}
