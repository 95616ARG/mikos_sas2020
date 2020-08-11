# Using AWS to get the data

As described in the paper, our results were produced using `r5.2xlarge` Amazon EC2 instances.
We used dedicated instances for stability.

We ran **200** instances simultaneously to get the numbers in a reasonable time.
EC2 Instances are spawned from a prebuild image with user-data to assign batches to run.
Instances access two S3 buckets to obtain the batch and save the results,
and they should have access permissions to the buckets through IAM role.

# Prerequisite 

The scripts uses awscli (>=1.16.142) and boto3 (>=1.9.132).
```
$ pip install --user awscli boto3
```

## AWS set up

### AMI

One must first generate an AMI to run the experiments.
Start with an empty `Ubuntu 18.04` AMI and download this artifact.

Then, run the following commands and save the image.
```
$ cd <PATH_TO_ARTIFACT>
$ sudo ./aws/dependencies.sh
$ ./setup.sh
$ echo 'export PATH="<PATH_TO_ARTIFACT>/build/run/bin:$PATH"' >> ~/.bashrc
$ echo 'export PYTHONPATH="<PATH_TO_ARTIFACT>:$PYTHONPATH"' >> ~/.bashrc
$ echo 'export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib"' >> ~/.bashrc
```

### S3 Buckets

**Create** **two** S3 buckets.
BUCKET1 stores the batch configurations, and BUCKET2 stores the experiment results.

[Creating S3 bucket](https://docs.aws.amazon.com/AmazonS3/latest/gsg/CreatingABucket.html)

### IAM role

**Create** an IAM role that gives access rights on the buckets to instances.
The instances uses this role to read from BUCKET1 and write to BUCKET2.

[Creating IAM role](https://docs.aws.amazon.com/IAM/latest/UserGuide/id_roles_create.html)

Go to IAM, policies. Create a policy using JSON. Edit the policy boilerplate below, substituting for POLICYNAMEHERE and BUCKETNAMEHERE. This policy will allow the role to read/write files in the S3 bucket.
Do this **two** times for BUCKET1 and BUCKET2 created above.
```
{
  "Version": "2012-10-17",
    "Statement": [
    {
      "Sid": "POLICYNAMEHERE",
      "Effect": "Allow",
      "Action": "s3:*",
      "Resource": "arn:aws:s3:::BUCKETNAMEHERE/*"
    },
    {
      "Sid": "VisualEditor1",
      "Effect": "Allow",
      "Action": "s3:ListAllMyBuckets",
      "Resource": "*"
    },
    {
      "Sid": "AllowRootListing",
      "Action": [
        "s3:ListBucket"
      ],
      "Effect": "Allow",
      "Resource": [
        "arn:aws:s3:::BUCKETNAMEHERE"
      ]
    }
  ]
}
```

The trust policy for your job role needs to extend to ec2 and ECS. Go to edit trust relationships and add,

```
{
  "Version": "2012-10-17",
  "Statement": [
  {
    "Effect": "Allow",
    "Principal": {
      "Service": "ec2.amazonaws.com"
    },
    "Action": "sts:AssumeRole"
  }
  ]
}
```

### Security group

**Create** a security group.

[Creating a security group](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/ec2-security-groups.html#creating-security-group)

### Key and Security group

**Create** a key pair.

[Creating key pair](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/ec2-key-pairs.html#having-ec2-create-your-key-pair).

# Reproducing data using AWS instances

## Step 1. Make batch files
```
$ mkdir batches
$ ./partition.py t1
$ ./partition.py t2
```

## Step 2. Upload the batch files to S3 BUCKET1.
```
## Change the BUCKET1 in script `upload_to_s3.sh` to your bucket.
$ ./upload_to_s3.sh
```

## Step 3. Run instances
This will run **200** `r5.2xlarge` instances in your account.

First run task T1.
```
## Change the REGION in script `run_instances.sh` to your AWS region.
$ ./run_instances.py t1 ami-092bba3fffa5292a7 your-security-group-id your-key-name your-iam-role-name your-BUCKET1-name your-BUCKET2-name 0 200
```

After above instances finish, run task T2.
```
## Change the REGION in script `run_instances.sh` to your AWS region.
$ ./run_instances.py t2 ami-092bba3fffa5292a7 your-security-group-id your-key-name your-iam-role-name your-BUCKET1-name your-BUCKET2-name 0 200
```

## Step 4. Download the results from BUCKET2 to `./results-ikos`.
```
## Change the BUCKET2 in script `download_from_s3.sh` to your bucket.
$ ./download_from_s3.sh
```

## Step 5. Generate CSV files
```
## Change the BUCKET2 in script `download_from_s3.sh` to your bucket.
$ ./download_from_s3.sh
$ ./gen_table.sh t1 results-ikos
$ ./gen_table.sh t2 results-ikos
```

CSV files `t1.csv` and `t2.csv` will be generated in `results-ikos/csv`.
Move them to `../data` and use them to generate figures and tables.

```
$ mkdir ../data
$ cp results-ikos/csv/* ../data
```
