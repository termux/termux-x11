import com.android.build.api.artifact.ArtifactTransformationRequest
import org.gradle.api.DefaultTask
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.InputFiles
import org.gradle.api.tasks.Internal
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.TaskAction

abstract class RenameApkTask extends DefaultTask {
    @InputFiles
    abstract DirectoryProperty getApkFolder()

    @OutputDirectory
    abstract DirectoryProperty getOutFolder()

    @Internal
    abstract Property<ArtifactTransformationRequest<RenameApkTask>> getTransformationRequest()

    @Input
    abstract Property<String> getNewFileName()

    @TaskAction
    void taskAction() {
        transformationRequest.get().submit(this) { it ->
            def outApkFile = new File(it.getOutputFile())
            outApkFile.renameTo(new File(apkFolder.get().asFile, newFileName.get()))
            outApkFile
        }
    }
}
