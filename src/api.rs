pub mod api {
    use actix_web::{get, post, web, HttpResponse, Responder};
    use serde::{Deserialize, Serialize};
    use std::process::Command;

    use std::fs;
    use std::fs::{File, OpenOptions};
    use std::io::prelude::*;
    use std::path::Path;

    #[derive(Debug, Serialize, Deserialize)]
    pub struct JudgeRequest {
        id: i32,
        user1_source: String,
        user2_source: String,
    }

    #[post("/judge-request")]
    pub fn judge_start_request(data: web::Json<JudgeRequest>) -> HttpResponse {
        let container_id: String = format!("judge{}", data.id);
        Command::new("docker")
            .args(vec![
                "run".to_string(),
                "-d".to_string(),
                "-m".to_string(),
                "2g".to_string(),
                "--cpus=1".to_string(),
                "--name".to_string(),
                container_id.clone(),
                "colledor-judge".to_string(),
            ])
            .output()
            .unwrap();
        
        println!("Hello");

        let src0_path: String = format!("./tmp/{}_0.cpp", data.id);
        let src1_path: String = format!("./tmp/{}_1.cpp", data.id);

        let mut f = File::create(&Path::new(&src0_path)).unwrap();
        f.write_all(data.user1_source.clone().as_bytes()).unwrap();
        let mut f = File::create(&Path::new(&src1_path)).unwrap();
        f.write_all(data.user2_source.clone().as_bytes()).unwrap();

        Command::new("gsed")
            .args(vec![
                "-i".to_string(),
                "s/PlayerXXX/Player1/".to_string(),
                src0_path.clone()
            ])
            .output()
            .unwrap();
        
        Command::new("gsed")
            .args(vec![
                "-i".to_string(),
                "s/PlayerXXX/Player2/".to_string(),
                src1_path.clone()
            ])
            .output()
            .unwrap();

        Command::new("docker")
            .args(vec![
                "cp".to_string(),
                src0_path.clone(),
                format!(
                    "{}:/judgedir/ac-library/atcoder/player1.cpp",
                    container_id.clone()
                ),
            ])
            .output()
            .unwrap();
        Command::new("docker")
            .args(vec![
                "cp".to_string(),
                src1_path.clone(),
                format!(
                    "{}:/judgedir/ac-library/atcoder/player2.cpp",
                    container_id.clone()
                ),
            ])
            .output()
            .unwrap();

        //fs::remove_file(src0_path.clone()).unwrap();
        //fs::remove_file(src1_path.clone()).unwrap();

        Command::new("docker")
            .args(vec![
                "exec".to_string(),
                "-d".to_string(),
                container_id.clone(),
                "bash".to_string(),
                "-c".to_string(),
                "./compile.sh".to_string(),
            ])
            .output()
            .unwrap();

        println!("START: {}",data.id);
        return HttpResponse::Ok().json({});
    }

    #[get("/judge-info/{id}")]
    pub fn judge_info(web::Path(id): web::Path<String>) -> HttpResponse {
        let container_id: String = format!("judge{}", id.clone());
        let op_path: String = format!("./tmp/output_{}.txt", id.clone());
        Command::new("docker")
            .args(vec![
                "cp".to_string(),
                format!("{}:/judgedir/judge_opt.txt", container_id.clone()),
                op_path.clone(),
            ])
            .output()
            .unwrap();

        #[derive(Debug, Serialize, Deserialize)]
        pub struct JudgeInfo {
            output: String,
        }
        let res = JudgeInfo {
            output: match fs::read_to_string(op_path.clone()){
                Err(err)=> "".to_string(),
                Ok(t) => {
                    fs::remove_file(op_path.clone()).unwrap();
                    t
                }
            }
        };
        return HttpResponse::Ok().json(res);
    }
    #[get("/judge-kill/{id}")]
    pub fn judge_kill(web::Path(id): web::Path<String>) -> HttpResponse {
        let container_id: String = format!("judge{}", id.clone());
        Command::new("docker")
            .args(vec![
                "rm".to_string(),
                "-f".to_string(),
                container_id.clone(),
            ])
            .output()
            .unwrap();
        println!("END: {}",id);
        return HttpResponse::Ok().json({});
    }
}
